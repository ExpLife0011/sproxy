#include "guest_s2.h"
#include "host.h"
#include "file.h"

Guest_s2::Guest_s2(Guest_s *const copy): Guest_s(copy) {
    windowsize = 65535;
    windowleft = 65535;
    struct epoll_event event;
    event.data.ptr = this;
    event.events = EPOLLIN | EPOLLOUT;
    epoll_ctl(efd, EPOLL_CTL_MOD, fd, &event);
    handleEvent = (void (Con::*)(uint32_t))&Guest_s2::defaultHE;
}



ssize_t Guest_s2::Read(void *buff, size_t size) {
    return Guest_s::Read(buff, size);
}


ssize_t Guest_s2::Write(Peer *who, const void *buff, size_t size)
{
    Http2_header header;
    memset(&header, 0, sizeof(header));
    if(idmap.left.count(who)){
        set32(header.id, idmap.left.find(who)->second);
    }else{
        who->clean(this, PEER_LOST_ERR);
        return -1;
    }
    set24(header.length, size);
    if(size == 0) {
        header.flags = END_STREAM_F;
        idmap.left.erase(who);
    }
    header.type = 0;
    SendFrame(&header, 0);
    int ret = Peer::Write(who, buff, size);
    this->windowsize -= ret;
    who->windowsize -= ret;
    return ret;
}



Http2_header* Guest_s2::SendFrame(const Http2_header *header, size_t addlen) {
    size_t len = sizeof(Http2_header) + addlen;
    Http2_header *frame = (Http2_header *)malloc(len);
    memcpy(frame, header, len);
    framequeue.push(frame);
    
    struct epoll_event event;
    event.data.ptr = this;
    event.events = EPOLLIN | EPOLLOUT;
    epoll_ctl(efd, EPOLL_CTL_MOD, fd, &event);
    return frame;
}


void Guest_s2::DataProc(Http2_header* header) {
    uint32_t id = get32(header->id);
    if(idmap.right.count(id)){
        Peer *host = idmap.right.find(id)->second;
        size_t len = get24(header->length);
        if(len > host->bufleft(this)){
            Reset(get32(header->id), ERR_FLOW_CONTROL_ERROR);
            host->clean(this, ERR_FLOW_CONTROL_ERROR);
            return;
        }
        host->Write(this, header+1, len);
        if((host->windowleft -= len) <= 100 * 1024){
            host->windowleft += ExpandWindowSize(id, 512*1024);
        }
        windowleft -= len;
    }else{
        Reset(get32(header->id), ERR_STREAM_CLOSED);
    }
}

void Guest_s2::ReqProc(HttpReqHeader &req)
{
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, sizeof(hostname));
    LOG("([%s]:%d):[%d] %s %s\n", sourceip, sourceport, req.id, req.method, req.url);
        
    if(req.hostname[0] && strcmp(req.hostname, hostname)){
        Host *host = new Host(req, this);
        host->windowsize = initalframewindowsize;
        host->windowleft = 512 *1024;
        idmap.insert(decltype(idmap)::value_type(host, req.id));
    }else {
        if(req.parse()){
            LOG("([%s]:%d):[%d] parse url failed\n", sourceip, sourceport, req.id);
            throw 0;
        }
        File *file = new File(req, this);
        file->windowsize = initalframewindowsize;
        idmap.insert(decltype(idmap)::value_type(file, req.id));
    }
}



void Guest_s2::Response(Peer *who, HttpResHeader &res)
{
    if(idmap.left.count(who)){
        res.id = idmap.left.find(who)->second;
    }else{
        who->clean(this, PEER_LOST_ERR);
        return;
    }
    res.del("Transfer-Encoding");
    res.del("Connection");
    char buff[FRAMELENLIMIT];
    SendFrame((Http2_header *)buff, res.getframe(buff, &request_table));
}


void Guest_s2::defaultHE(uint32_t events)
{
    if (events & EPOLLERR || events & EPOLLHUP) {
        int       error = 0;
        socklen_t errlen = sizeof(error);

        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (void*)&error, &errlen) == 0) {
            LOGE("([%s]:%d): guest_s error:%s\n",
                  sourceip, sourceport, strerror(error));
        }
        clean(this, INTERNAL_ERR);
        return;
    }
    
    if (events & EPOLLIN) {
        (this->*Http2_Proc)();
        if(windowleft < 50 *1024 *1024){
            windowleft += ExpandWindowSize(0, 50*1024*1024);
        }
    }

    if (events & EPOLLOUT) {
        if (dataleft) {
            if(writelen){ //先将data帧的数据写完
                int ret = Guest_s::Write(wbuff, Min(writelen,dataleft));
                if(ret>0){
                    memmove(wbuff, wbuff + ret, writelen - ret);
                    writelen -= ret;
                    dataleft -= ret;
                }else if (showerrinfo(ret, "guest_s2 write error")) {
                    clean(this, WRITE_ERR);
                    return;
                }
                for(auto i:waitlist){
                    i->writedcb();
                }
                waitlist.clear();
            }else{
                return;
            }
        }
        if(dataleft == 0 && !framequeue.empty()){  //data帧已写完
            do{
                Http2_header *header = framequeue.front();
                size_t framewritelen;
                if(header->type){
                    framewritelen = get24(header->length) + sizeof(Http2_header);
                }else{
                    framewritelen = sizeof(Http2_header);
                }
                frameleft = frameleft?frameleft:framewritelen;
                int ret = Guest_s::Write((char *)header+framewritelen-frameleft, frameleft);
                if(ret>0){
                    frameleft -= ret;
                    if(frameleft == 0 ){
                        size_t len = get24(header->length);
                        framequeue.pop();
                        if(header->type == 0 && len){
                            dataleft = len;
                            free(header);
                            break;
                        }
                        free(header);
                    }
                }else{
                    if (showerrinfo(ret, "guest_s2 write error")) {
                        clean(this, WRITE_ERR);
                    }
                    break;
                }
            }while(!framequeue.empty());
        }

        if (writelen == 0 && framequeue.empty()) {
            struct epoll_event event;
            event.data.ptr = this;
            event.events = EPOLLIN;
            epoll_ctl(efd, EPOLL_CTL_MOD, fd, &event);
        }
    }
}


void Guest_s2::RstProc(uint32_t id, uint32_t errcode) {
    if(idmap.right.count(id)){
        if(errcode)
            LOGE("([%s]:%d): reset stream [%d]: %d\n", sourceip, sourceport, id, errcode);
        idmap.right.find(id)->second->clean(this, errcode);
        idmap.right.erase(id);
    }
}


void Guest_s2::WindowUpdateProc(uint32_t id, uint32_t size) {
    LOG("get a window update frame[%u]: %u\n", id, size);
    if(id){
        if(idmap.right.count(id)){
            LOG("current frame window size[%u]:%lu\n",id, idmap.right.find(id)->second->windowsize);
            idmap.right.find(id)->second->windowsize += size;
        }
    }else{
        LOG("current connection window size:%lu\n", windowsize);
        windowsize += size;
    }
}


void Guest_s2::GoawayProc(Http2_header* header) {
    clean(this, get32(header+1));
}

void Guest_s2::ErrProc(int errcode) {
    Guest::ErrProc(errcode);
}

void Guest_s2::AdjustInitalFrameWindowSize(ssize_t diff) {
    for(auto i: idmap.left){
       i.first->windowsize += diff; 
    }
}

void Guest_s2::clean(Peer *who, uint32_t errcode)
{
    if(who == this) {
        Peer::clean(who, errcode);
    }else if(idmap.left.count(who)){
        Reset(idmap.left.find(who)->second, errcode>30?ERR_INTERNAL_ERROR:errcode);
        idmap.left.erase(who);
    }
    waitlist.erase(who);
}

size_t Guest_s2::bufleft(Peer *peer) {
    size_t windowsize = Min(peer->windowsize, this->windowsize);
    return Min(windowsize, Peer::bufleft(peer));
}

void Guest_s2::wait(Peer *who){
    waitlist.insert(who);
    Peer::wait(who);
}
