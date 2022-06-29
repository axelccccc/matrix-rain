/**
 * @file matrix_rain.cpp
 * @author axelccccc (github.com/axelccccc)
 * @brief matrix rain
 * @version 0.1
 * @date 2022-03-25
 * 
 * @copyright Copyright (c) axelccccc — 2022
 * 
 * @todo organize / refactor / document the code
 * 
 * @bug Occasional segmentation faults : the smaller the window size,
 * the more often and faster they occur
 * 
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <iterator>
#include <random>
#include <limits>
#include <thread>
#include <deque>
#include <set>
#include <csignal>
#include <cstdlib>
#include <unistd.h>
#include <sys/ioctl.h>


/**
 * @brief Displays the number of trails currently alive
 */
// #define DEBUG

/**
 * @brief Refresh rate of the CLI screen — general speed of the program
 */
#define REFRESH_RATE    40ms


/**
 * @brief To use ms
 */
using namespace std::chrono_literals;

/**
 * @brief typedef for a grid of characters — CLI ascii-based screen
 */
using Display = std::vector<std::vector<char>>;






/**
 * @brief Characters used in the trails
 * @note This is just a constant for the program,
 * the characters are however passed as a parameter 
 * to the MatrixRain object
 */
static const std::string characters = "&@#$*%£=+?";
/**
 * @brief Static variable checked by the main matrix rain
 * function ( `start()` ) to know when to stop the program.
 * It is switched by a signal handler function.
 * @note maybe should it be made part of the MatrixRain class ?
 */
static bool stop_requested = false;


/**
 * @brief Signal handler function to stop the program.
 * It switches on the `stop_requested` variable, which is
 * periodically checked by the `start()` function.
 * @param signal Received signal ( `SIGINT` )
 */
void signal_handler(int signal) {
    stop_requested = true;
}



class RandomGenerator {
public:

    RandomGenerator()
    : gen(rd()), dist(0.0, 1.0) {} 

    template<typename T>
    T operator()(T min, T max) {
        return (T)(dist(gen) * (max - min));
    }

    template<typename T>
    int chance(T probability) {
        return dist(gen) <= probability;
    }

private:

    std::random_device rd;
    std::mt19937 gen;
    std::uniform_real_distribution<> dist;

};




class Trail {
public:

    struct Particle {

        int x;
        int y;

        // int color; // maybe color is not an int ?

    };

    Trail(int length, double speed ,int start_x, int start_y) 
        : max_length(length), speed(speed) {
        trail.push_front({start_x, start_y});
    }

    void advance() {
        advance_count += speed;
        if(advance_count >= 1.0) {
            advance_by(std::floor(advance_count));
            advance_count -= std::floor(advance_count);
        }
    }

    int& x() {
        return trail.front().x;
    }

    int& y() {
        return trail.front().y;
    }

    Particle& last() {
        return trail.back();
    }

// private:

    int max_length;
    std::deque<Particle> trail;
    double speed;

    double advance_count { 0.0 };

    void advance_by(int n) {
        for(int i = 0; i < n; i++) {
            /* for(auto& p : trail) {
                p.darken(); // Function to darken the color by (max / current length)
            } */
            trail.push_front({trail.front().x, trail.front().y + 1});
            if(trail.size() > max_length) {
                trail.pop_back();
            }
        }
    }


};



struct WindowDimensions {
    int char_rows;
    int char_columns;
};



WindowDimensions get_window_dimensions() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return { w.ws_row, w.ws_col };
}



struct MatrixRainParameters {
    int height;
    int width;
    std::string characters;
    /* <type> color */
    double density;
    double speed;
    int max_trail_length;
};



class MatrixRain {
public:

    MatrixRain(const MatrixRainParameters& params)
        :   width(params.width), 
            height(params.height),
            characters(params.characters),
            density(params.density), 
            speed(params.speed),
            max_trail_length(params.max_trail_length) {
                display = Display(height, std::vector<char>(width, ' '));
        }

        void test() {

            while(!stop_requested) {

                if(r.chance(density)) {
                    spawn_trails(r(1, (int)(20 * density)));
                }

                update_display();

                system("clear");
                print();

                update_positions();

                std::this_thread::sleep_for(REFRESH_RATE);

            }

        }

private:

    Display display;

    std::set<std::unique_ptr<Trail>> trails;

    std::string characters;

    int width, height;

    double density;
    double speed;

    int max_trail_length;

    RandomGenerator r;

    void spawn_trails(int n) {
        for(int i = 0; i < n; ++i) {
            spawn_trail();
        }
    }

    /**
     * @brief 
     * @note The `-max_trail_length` allow trails to spawn not too far out above 
     * the screen for a more homogeneous effect
     */
    void spawn_trail() {
        trails.emplace( std::make_unique<Trail>(
                            r(1, max_trail_length),
                            r(speed * 1.0, speed * 3.0),
                            r(0, width), 
                            r(-max_trail_length, height)));
    }

    void update_display() {
        clear();
        for(auto& t : trails) {
            if(t->last().y > height) { 
                trails.erase(t); 
                continue; 
            }
            for(auto& p : t->trail) {
                if(p.x < width && p.y < height) {
                    display[p.y][p.x] = characters[r(0, (int)characters.size() - 1)];
                }
            }
        }
    }

    void update_positions() {
        for(auto& t : trails) {
            t->advance();
        }
    }

    void print() {
        for(auto& row : display) {
            std::copy(row.begin(), row.end(), std::ostream_iterator<char>(std::cout));
            std::cout << '\n';
        }
        #ifdef DEBUG
        std::cout << "Alive trails : " << trails.size() << '\n';
        #endif
    }

    void clear() {
        for(auto& row : display) {
            std::fill(row.begin(), row.end(), ' ');
        }
    }

};



int main() {

    std::signal(SIGINT, signal_handler);

    MatrixRainParameters params;

    auto [display_height, display_width] = get_window_dimensions();

    params.height = display_height - 2;
    params.width = display_width;
    params.characters = characters;
    params.density = 0.35;
    params.speed = 1.0;
    params.max_trail_length = 50;

    MatrixRain matrix(params);

    matrix.test();

}