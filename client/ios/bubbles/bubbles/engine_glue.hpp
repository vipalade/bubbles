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
    
    typedef void (*ExitCallbackT)(void*);
    typedef void (*GUIUpdateCallbackT)(void*);
    typedef void (*AutoUpdateCallbackT)(void*,int, int);
    
    int engine_start(
        const char  *_host,
        const char  *_room,
        const int   _secure,
        const int   _compress,
        const int   _auto_pilot,
        const char  *_ssl_verify_authority,
        const char  *_ssl_client_cert,
        const char  *_ssl_client_key,
        void *_pexit_data, ExitCallbackT _exit_fnc,
        void *_pupdate_data, GUIUpdateCallbackT _update_fnc,
        void *_pauto_data, AutoUpdateCallbackT _auto_fnc
    );
    
    void engine_move(int _x, int _y);
    
    void engine_plot_start();
    void engine_plot_done();
    int engine_plot_end();
    void engine_plot_next();
    int engine_plot_x();
    int engine_plot_y();
    int engine_plot_color();
    int engine_plot_my_color();
    
    int engine_scale_x(int _x, int _w);
    int engine_scale_y(int _y, int _h);
    
    int engine_reverse_scale_x(int _x, int _w);
    int engine_reverse_scale_y(int _y, int _h);
#ifdef __cplusplus
}
#endif

#endif /* engine_glue_hpp */
