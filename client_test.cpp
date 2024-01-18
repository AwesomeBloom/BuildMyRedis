//
// @birth: created by windy on 2023/12/25.
//

#include <cstdio>

#include "src/client/basic_client.hpp"
#include "src/client/protocol_parsing_client.hpp"
#include "src/client/event_loop_client.hpp"
#include "src/client/basic_full_client.hpp"

int main(int argc, char *argv[]) {

    auto work = [argc, argv](){
        printf("Client initializing... ");
        BasicFullClient client;
        printf("done! \n");

        int client_ret = client.work(argc, argv);
        return client_ret;
    };

    return work();
}