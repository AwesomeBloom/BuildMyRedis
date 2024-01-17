//
// @birth: created by windy on 2023/12/25.
//

#include <cstdio>

#include "src/client/BasicClient.hpp"
#include "src/client/ProtocolParsingClient.hpp"
#include "src/client/EventLoopClient.hpp"
#include "src/client/BasicFullClient.hpp"

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