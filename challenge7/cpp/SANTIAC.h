#pragma once

#include <vector>
#include <queue>
#include <iostream>
#include <string>

class SANTIAC {
public:
    enum class PARAMETER_MODE {position, immediate};
    enum class RUN_MODE {running, terminated};

    SANTIAC(std::istream& input_stream);

    int execute();
    void step();

    void op_add(PARAMETER_MODE, PARAMETER_MODE);
    void op_mult(PARAMETER_MODE, PARAMETER_MODE);
    void op_read();
    void op_print(PARAMETER_MODE);
    void op_jump_if_true(PARAMETER_MODE, PARAMETER_MODE);
    void op_jump_if_false(PARAMETER_MODE, PARAMETER_MODE);
    void op_is_less_than(PARAMETER_MODE, PARAMETER_MODE);
    void op_is_equal(PARAMETER_MODE, PARAMETER_MODE);

    struct IntCode {
        IntCode(int input);
        int operation;
        SANTIAC::PARAMETER_MODE mode[3];
        friend std::ostream& operator<<(std::ostream& out, const IntCode& c);
    };

    // I used vim macros to make this list lol
    void set_thrusters(int input[5]);
    static int configs[92][5];
private:
    int m_head;
    RUN_MODE m_status;
    std::vector<int> m_data;
    std::queue<int> m_input;

    int thrusters();
    int fetch(PARAMETER_MODE mode, int offset);
    void dump_data();

    struct thrusters {
        bool ret_settings = false;
        int phase_settings[5];
        int phase = 0;
        int prev_output = 0;
    } m_thrusters;
};
