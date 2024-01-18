//
// @birth: created by Tianyi on 2023/12/25.
//

#include <cstdio>
#include "src/server/basic_server.hpp"
#include "src/server/protocol_parsing_server.hpp"
#include "src/server/event_loop_server.hpp"
#include "src/server/basic_full_server.hpp"

int main(int argc, char *argv[]) {

    auto work = [](){
        printf("Server initializing... ");
        BasicFullServer server;
        printf("done! \n");

        int server_ret = server.work();
        if (server_ret < 0) return server_ret;
        return 0;
    };

    return work();
}