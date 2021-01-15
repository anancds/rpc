// 0. 基础代码
 
//       // 设置某fd为O_NONBLOCK模式
//       int set_non_block(int fd);

// // server端socket流程:socket(),setsockopt(),bind(),listen(),set_non_block()，返回server_fd
// int setup_tcp_server(int port);

// // client端socket流程:socket(),connect()，返回连接的sockfd
// int create_io_channel(const char *ipaddr, int port);

// 1. 搭建TCP Server
//       下面以伪代码方式给出，错误处理省略
 
//       int main(int argc, char *argv[])
//       {
//   // 初始化
//   …

//     // event初始化
//     event_init();
//   init_server(port, params…);
//   event_dispatch();

//   return 0;
// }

// int init_server(short port, params…) {
//   int listen_fd = setup_tcp_server(port);
//   set_non_block(listen_fd);

//   // 将输入的参数params… 组织为一个结构，以指针的方式存于accept_param
//   struct event *ev_accept = (struct event *)malloc(sizeof(struct event));
//   event_set(ev_accept, listen_fd, EV_READ | EV_PERSIST, on_accept, (void *)accept_param);
//   event_add(ev_accept, NULL);

//   return 0;
// }

// void on_accept(int fd, short ev, void *arg) {
//   int client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
//   set_non_block(client_fd);

//   // Disable the Nagle (TCP No Delay) algorithm
//   int flag = 1;
//   int ret = setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag));

//   // 设置buffer event
//   // 假设client_t有一个类型为struct bufferevent*的域buf_ev
//   //     total_len: 指示请求总长度
//   //     cur_size:  指示当前已经接收的请求长度
//   //     data:      请求数据本身
//   client_t *client = (client_t *)malloc(sizeof(client_t));
//   client->buf_ev = bufferevent_new(client_fd, buffer_on_read, buffer_on_write, buffer_on_error, client);
//   client->total_len = 0;
//   client->cur_size = 0;
//   client->data = NULL;
//   bufferevent_enable(client->buf_ev, EV_READ | EV_WRITE);
// }

// void buffer_on_read(struct bufferevent *ev_buf, void *opqaue) {
//   // 读请求并处理

//   client_t *client = (client_t *)opaque;
//   size_t rdsz;
//   int sz;

//   if (client->total_len <= client->cur_size) {
//     // 读新请求

//     // 获得新请求的总长度
//     rdsz = bufferevent_read(ev_buf, &sz, sizeof(int));
//     client->total_len = sz;

//     // 开始读新请求数据(一次不一定能读完!)
//     char *data = (char *)malloc(sz);
//     rdsz = bufferevent_read(ev_buf, data, sz);
//     client->cur_size = (int)rdsz;
//     client->data = data;
//   } else {
//     // 继续读该请求
//     rdsz = bufferevent_read(ev_buf, client->data + client->cur_size, client->total_len - client->cur_size);
//     client->cur_size += (int)rdsz;
//   }

//   if (client->cur_size >= client->total_len) {
//     // 处理该(完整的)请求
//     request_t req;   // 请求的数据结构(通过protobuf定义)
//     response_t res;  // 回应的数据结构(通过protobuf定义)

//     // 调用处理函数对request进行处理,并把结果写到response中
//     req.ParseFromArray((const void *)client->data, client->total_len);
//     process_func(req, res);

//     // 写回应
//     string output;
//     res.SerializeToString(&output);
//     int status = bufferevent_write(ev_buf, output.c_str(), output.length());
//   }
// }

// void buffer_on_write(struct bufferevent *ev_buf, void *opqaue) {
//   // 作清理工作

//   client_t *client = (client_t *)opaque;
//   if (client->data && strlen(client->data) != 0) free(client->data);
//   client->data = NULL;
//   client->total_len = 0;
//   client->cur_size = 0;
// }

// void buffer_on_error(struct bufferevent *ev_buf, short what, void *opqaue) {
//   // 给出错误信息
//   division_client *client = (division_client *)opaque;
//   struct sockaddr_in client_addr;
//   socklen_t len = sizeof(client_addr);
//   if (getpeername(client->sock_fd, (struct sockaddr *)&client_addr, &len) == 0)
//     LOG_INFO(“Client(% s
//                      : % u) connection closed or
//                Error occured — % d\n”,
//              inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), what);
//   else
//     LOG_INFO(“Client(unknown) connection closed or Error occured — % d\n”, what);

//   if (client) {
//     if (client->data) free(client->data);
//     free(client);
//   }
// }
