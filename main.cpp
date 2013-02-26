#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <thread>
#include <fstream>

#include <itpp/signal/source.h>
#include <itpp/signal/sigfun.h>
#include <itpp/base/circular_buffer.h>
#include <itpp/stat/misc_stat.h>

#include <Fl/Fl.H>
#include <Fl/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Output.H>

#include <SDL/SDL.h>

#include <zmq.hpp>

using namespace std;
using namespace itpp;



int main()
{

    atexit(SDL_Quit);

    //if( SDL_Init(SDL_INIT_VIDEO) < 0 ) return(1);
    SDL_WM_SetCaption("Uruk - NSPLab", NULL);
    SDL_Surface* screen = SDL_SetVideoMode( 800, 480, 32, SDL_DOUBLEBUF|SDL_ANYFORMAT);

    float alpha = 0.995;
    float beta = 0.1;
    bool exit = false;
    thread gui(runGUI, &alpha, &beta, &exit);

    zmq::context_t context(1);
    zmq::socket_t subscriber (context, ZMQ_SUB);
    subscriber.connect("tcp://192.168.56.101:5556");
    subscriber.setsockopt(ZMQ_SUBSCRIBE, "", 0);


    Sine_Source sin_gen_30hz(0.0025, 0.0);
    Sine_Source sin_gen_30hz_2(0.0025, 0.0, 3.0);
    Sine_Source sin_gen_15hz(0.0050, 0.0);
    vec sin_30hz = sin_gen_30hz(1200);
    vec sin_15hz = sin_gen_15hz(1200);
    vec sin_30hz_2 = sin_gen_30hz_2(1200);

    vec mixed_signal = concat(sin_30hz, sin_15hz);

    timeval t1, t2;
    gettimeofday(&t1, NULL);

    vec spd1 = spectrum(mixed_signal, 1200.0);

    gettimeofday(&t2, NULL);

    double elapsedTime;
    elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
    elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;
    cout << elapsedTime << " ms\n";

    vec spd2 = spectrum(sin_15hz(1,900), 1200.0);
    vec spd3 = spectrum(sin_30hz(1,900), 1200.0);
    //for (int i=0; i<10/*spd1.length()*/; i++) {
    //    cout<<i<<" "<<spd1(i)<<" \t"<<spd2(i)<<" \t"<<spd3(i)<<endl;
    //}

    ofstream csv("left_power_data.csv");
    ofstream r_csv("right_power_data.csv");

    // SDL section
    SDL_Rect redRect;
    redRect.x = 375;
    redRect.y = 420;
    redRect.h = 50;
    redRect.w = 50;

    SDL_Rect blueRect;
    blueRect.x = 575;
    blueRect.y = 420;
    blueRect.h = 50;
    blueRect.w = 50;

    // --ymin -0.5
    FILE* gfeed = popen("feedgnuplot --lines --nodomain --ymin -0.5 --ymax 4.5 --legend 0 \"right mu power\" --legend 1 \"right mu power\" --legend 2 \"left beta power\" --legend 3 \"right beta power\" --stream -xlen 400 --geometry 940x450-0+0", "w");

    float channels[65];
    string str;
    size_t counter = 0;

    float left_av_mu_power = 0.0;
    float right_av_mu_power = 0.0;
    float left_av_beta_power = 0.0;
    float right_av_beta_power = 0.0;

    Circular_Buffer<double> cb1(66);
    Circular_Buffer<double> cb2(66);

    for (size_t i=0; i<66; i++) {
        cb1.put(0.0);
        cb2.put(0.0);
    }

    Vec<double> left_signal;
    Vec<double> right_signal;

    cb1.put(1.0);cb1.put(2.0);cb1.put(3.0);cb1.put(4.0);
    cb1.peek(left_signal);
    cout<<left_signal<<endl;

    Circular_Buffer<double> cb_power(200);

    bool gostate = false;

    size_t wait_time = rand() % 10 + 10;
    size_t go_time = rand() % 3 + 10;

    gettimeofday(&t1, NULL);

    while (!exit) {

        gettimeofday(&t2, NULL);
        elapsedTime = (t2.tv_sec - t1.tv_sec);

        if (gostate) {
            if (elapsedTime > go_time) {
                gostate = false;
                gettimeofday(&t1, NULL);
                wait_time = rand() % 6 + 12;
            }
        } else {
            if (elapsedTime > wait_time) {
                gostate = true;
                gettimeofday(&t1, NULL);
                go_time = rand() % 6 + 12;
            }
        }


        zmq::message_t update;
        subscriber.recv(&update);
        std::istringstream iss(static_cast<char*>(update.data()));

        for (int i=0; i<65; i++) {
            iss >> str;
            channels[i] = atof(str.c_str());
        }

        cb1.put(channels[28] - 0.25 * (channels[26]+channels[10]+channels[30]+channels[46]) );
        cb2.put(channels[32] - 0.25 * (channels[30]+channels[14]+channels[34]+channels[50]) );

        counter += 1;
        // plot a single channel
        if (counter == 13) {
            counter = 0;

            cb1.peek(left_signal);
            cb2.peek(right_signal);

            vec left_spd = spectrum(left_signal, 256.0);
            vec right_spd = spectrum(right_signal, 256.0);

            csv<<redRect.y<<","<<gostate<<","<<left_spd(0);
            for (size_t t=0; t<left_spd.length(); t++)
                csv<<","<<left_spd(t);
            csv<<endl;
            csv.flush();

            r_csv<<redRect.y<<","<<gostate<<","<<right_spd(0);
            for (size_t t=0; t<right_spd.length(); t++)
                r_csv<<","<<right_spd(t);
            r_csv<<endl;
            r_csv.flush();

            //for (size_t t=0; t<left_spd.length(); t++)
            //    cout<<" ["<<t<<"]: "<<left_spd(t);
            //cout<<endl;

            float left_mu_power = 0.0f;
            float right_mu_power = 0.0f;
            float left_beta_power = 0.0f;
            float right_beta_power = 0.0f;
            for (size_t i=8; i<=12; i++) {
                left_mu_power += left_spd(i)/5.0;
                right_mu_power += right_spd(i)/5.0;

                left_av_mu_power = (alpha) * left_av_mu_power + (1.0 - alpha) * left_mu_power;
                right_av_mu_power = (alpha) * right_av_mu_power + (1.0 - alpha) * right_mu_power;
            }
            for (size_t i=18; i<=26; i++) {
                left_beta_power += left_spd(i)/5.0;
                right_beta_power += right_spd(i)/5.0;

                left_av_beta_power = (alpha) * left_av_beta_power + (1.0 - alpha) * left_beta_power;
                right_av_beta_power = (alpha) * right_av_beta_power + (1.0 - alpha) * right_beta_power;
            }

            cb_power.put(right_av_mu_power);

            fprintf(gfeed, "%f \n", right_av_mu_power/*, right_av_mu_power, left_av_beta_power, right_av_beta_power*/);
            //printf("%f %f \n", left_av_power, right_av_power);
            fprintf(gfeed, "replot\n");
            fflush(gfeed);
        }

        vec power_vec;
        cb_power.peek(power_vec);
        float mean_power = mean(power_vec);

        if (mean_power > 2.0)
            mean_power = 2.0;

        //int tmp = 420 - (2.0-(mean_power))/2.0 * 450.0;
        //if (tmp > 100)
         //   redRect.y == tmp;
        redRect.y = 360 - (2.0-(mean_power))/2.0 * 300.0;

        //csv<<","<<redRect.y<<","<<gostate<<endl;
        //r_csv<<","<<redRect.y<<","<<gostate<<endl;
        //csv.flush();
        //r_csv.flush();

        /*if (mean_power < 0.65)
            redRect.y = 140;
        else
            redRect.y = 420;*/


        // SDL section
        SDL_FillRect(screen , NULL , 0x221122);
        SDL_Rect workRect;
        workRect.y = 90;
        workRect.x = 365;
        workRect.w = 70;
        workRect.h = 310;

        SDL_Rect workRect2;
        workRect2.y = 95;
        workRect2.x = 370;
        workRect2.w = 60;
        workRect2.h = 300;

        SDL_Rect hRect;
        hRect.y = 245;
        hRect.x = 350;
        hRect.w = 100;
        hRect.h = 2;

        SDL_FillRect(screen , &workRect , SDL_MapRGB(screen->format , 200 , 200 , 200 ) );
        SDL_FillRect(screen , &workRect2 , SDL_MapRGB(screen->format , 34 , 17 , 34 ) );

        SDL_FillRect(screen , &hRect , SDL_MapRGB(screen->format , 34 , 100 , 34 ) );


        SDL_FillRect(screen , &redRect , SDL_MapRGB(screen->format , 200 , 20 , 200 ) );
        //SDL_FillRect(screen , &blueRect , SDL_MapRGB(screen->format , 20 , 20 , 200 ) );
        if (gostate) {
            fill_circle(screen, 50,50, 10, 0xffff0000);
        } else {
            fill_circle(screen, 50,50, 10, 0xff00ff00);
        }
        SDL_Flip(screen);
    }


    /*for (int i=0; i<mixed_signal.length(); i++) {
        fprintf(gfeed, "%f \n", mixed_signal(i));
        fprintf(gfeed, "replot\n");
        fflush(gfeed);

        if (i == 0)
            usleep(20000);
        else
            usleep(200);
    }*/



    //pclose(gfeed);
    gui.join();


    return 0;
}

