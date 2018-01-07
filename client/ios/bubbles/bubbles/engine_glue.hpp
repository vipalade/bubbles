//
//  engine_glue.hpp
//  bubbles
//
//  Created by Valentin Palade on 07/01/2018.
//  Copyright © 2018 Valentin Palade. All rights reserved.
//

#ifndef engine_glue_hpp
#define engine_glue_hpp

#include <stdio.h>


#ifdef __cplusplus
extern "C"{
#endif
    
    int engine_start(
        const char *_host,
        const char *_room,
        const int _secure,
        const int _compress,
        const int _auto_pilot,
        const char *_ssl_verify_authority,
        const char *_ssl_client_cert,
        const char *_ssl_client_key
    );
    
#ifdef __cplusplus
}
#endif

#endif /* engine_glue_hpp */
