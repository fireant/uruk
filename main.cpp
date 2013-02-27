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

#include "utils.h"
#include "gnuplot.h"
#include "eeg_receiver.h"

using namespace std;
using namespace itpp;


int main()
{
    // alpha value in first order autoregressive
    float alpha = 0.995;

    // signals gui window to close
    bool exit = false;

    // the ball/cursor whose vertical position(redRect.y) is controlled by EEG
    SDL_Rect redRect;
    redRect.x = 375;
    redRect.y = 420;
    redRect.h = 50; // height
    redRect.w = 50; // width


    // * SDL - graphics
    // register exit function of SDL to release taken memory gracefully
    atexit(SDL_Quit);
    // title of window
    SDL_WM_SetCaption("Uruk - NSPLab", NULL);
    // create window of 800 by 480 pixels
    SDL_Surface* screen = SDL_SetVideoMode( 800, 480, 32, SDL_DOUBLEBUF|SDL_ANYFORMAT);
    // dialog box to modify alpha
    thread gui(runGUI, &alpha, &exit);

    // receive EEG signal using ZMQ
    EegReceiver eeg;

    // log files
    ofstream l_csv("left_power_data.csv");
    ofstream r_csv("right_power_data.csv");

    // gnuplot window to plot live data
    GnuPlot gnuplot;


    float channels[65];
    string str;
    size_t counter = 0;

    float left_av_mu_power = 0.0;
    float right_av_mu_power = 0.0;
    float left_av_beta_power = 0.0;
    float right_av_beta_power = 0.0;

    // circular buffers to hold filtered eeg values, last 66 values
    Circular_Buffer<double> cb_eeg_left(66);
    Circular_Buffer<double> cb_eeg_right(66);
    // initialize circular buffers
    for (size_t i=0; i<66; i++) {
        cb_eeg_left.put(0.0);
        cb_eeg_right.put(0.0);
    }

    // circular buffer to hold filtered power, last 200 values
    Circular_Buffer<double> cb_power(200);


    // if true: exercise period, if false: relax period
    bool gostate = false;

    // random length of wait rest and exercise periods
    size_t wait_time = rand() % 10 + 10;
    size_t go_time = rand() % 3 + 10;

    gettimeofday(&t1, NULL);

    // main loop
    while (!exit) {

        // measure time length of current relax/exercise period
        gettimeofday(&t2, NULL);
        elapsedTime = (t2.tv_sec - t1.tv_sec);

        // check if we have to toggle mode, relax/exercise
        if (gostate) {
            if (elapsedTime > go_time) {
                gostate = false;
                gettimeofday(&t1, NULL);
                // relax period of 12 to 18 seconds
                wait_time = rand() % 6 + 12;
            }
        } else {
            if (elapsedTime > wait_time) {
                gostate = true;
                gettimeofday(&t1, NULL);
                // exercise period of 12 to 18 seconds
                go_time = rand() % 6 + 12;
            }
        }

        // receive EEG
        eeg.receive(channels);

        // large laplacian filter of C3
        cb_eeg_left.put(channels[28] - 0.25 * (channels[26]+channels[10]+channels[30]+channels[46]) );
        // large laplacian filter of C4
        cb_eeg_right.put(channels[32] - 0.25 * (channels[30]+channels[14]+channels[34]+channels[50]) );

        // every 13 cycles ~ 50 ms (= 13/256Hz) compute spectrum
        counter += 1;
        if (counter == 13) {
            counter = 0;

            // get values circular buffers in vectors
            Vec<double> left_signal;
            Vec<double> right_signal;
            cb_eeg_left.peek(left_signal);
            cb_eeg_right.peek(right_signal);

            // compute spectrum of signals, 256 Hz sampling rate
            vec left_spd = spectrum(left_signal, 256.0);
            vec right_spd = spectrum(right_signal, 256.0);

            // TODO: write a function for this
            l_csv<<redRect.y<<","<<gostate<<","<<left_spd(0);
            for (size_t t=0; t<left_spd.length(); t++)
                l_csv<<","<<left_spd(t);
            l_csv<<endl;
            l_csv.flush();

            r_csv<<redRect.y<<","<<gostate<<","<<right_spd(0);
            for (size_t t=0; t<right_spd.length(); t++)
                r_csv<<","<<right_spd(t);
            r_csv<<endl;
            r_csv.flush();

            float left_mu_power = 0.0f;
            float right_mu_power = 0.0f;
            float left_beta_power = 0.0f;
            float right_beta_power = 0.0f;
            for (size_t i=8; i<=12; i++) {
                left_mu_power += left_spd(i)/5.0;
                right_mu_power += right_spd(i)/5.0;
            }
            left_av_mu_power = (alpha) * left_av_mu_power + (1.0 - alpha) * left_mu_power;
            right_av_mu_power = (alpha) * right_av_mu_power + (1.0 - alpha) * right_mu_power;

            for (size_t i=18; i<=26; i++) {
                left_beta_power += left_spd(i)/5.0;
                right_beta_power += right_spd(i)/5.0;
            }
            left_av_beta_power = (alpha) * left_av_beta_power + (1.0 - alpha) * left_beta_power;
            right_av_beta_power = (alpha) * right_av_beta_power + (1.0 - alpha) * right_beta_power;

            cb_power.put(right_av_mu_power);

            // update plot
            gnuplot.plot(right_av_mu_power);
        }

        vec power_vec;
        cb_power.peek(power_vec);
        float mean_power = mean(power_vec);

        if (mean_power > 2.0)
            mean_power = 2.0;

        redRect.y = 360 - (2.0-(mean_power))/2.0 * 300.0;

        // SDL section
        DrawGraphics(screen, gostate, &redRect);
    }

    gui.join();

    return 0;
}

