#include "SANTIAC.h"

SANTIAC::SANTIAC(std::istream& input_stream) {
    for (std::string int_str; std::getline(input_stream, int_str, ',');) {
        long num = stol(int_str);
        m_data.push_back(num);
    }
    while (m_data.size() < 30000) m_data.push_back(0);
    m_head = 0;
    m_base = 0;

    m_io.readMe.push(2);
}

long SANTIAC::execute() {
    if (m_status == RUN_MODE::unstarted) m_head = 0;
    m_status = RUN_MODE::running;

    while (m_status == RUN_MODE::running) {
        step();
    }

    return m_io.prev_output;
}

void SANTIAC::dump_data() {
    if (!m_flags.debug_dump) return;
    for (long i = 0; i < 900; i++) {
        if (m_head == i) std::cout << "!>";
        std::cout << m_data[i] << ",";
    }
    std::cout << std::endl;
    if (m_flags.debug) std::cout << std::endl;
}

void SANTIAC::step() {
    IntCode c(m_data[m_head]);
    if (m_flags.debug) {
        std::cout << "\n" << "[" << m_head << "]: Recv'd IntCode " << c << std::endl;
    }

    switch (c.operation) {
        case 1: op_add(c.mode[0], c.mode[1], c.mode[2]); break;
        case 2: op_mult(c.mode[0], c.mode[1], c.mode[2]); break;
        case 3: op_read(c.mode[0]); break;
        case 4: op_print(c.mode[0]); break;
        case 5: op_jump_if_true(c.mode[0], c.mode[1]); break;
        case 6: op_jump_if_false(c.mode[0], c.mode[1]); break;
        case 7: op_is_less_than(c.mode[0], c.mode[1], c.mode[2]); break;
        case 8: op_is_equal(c.mode[0], c.mode[1], c.mode[2]); break;
        case 9: op_adjust_relative_base(c.mode[0]); break;
        case 99:
                //std::cout << "Program terminated." << std::endl;
                //std::cout << "m_data[0] = " << m_data[0] << std::endl;
                m_status = RUN_MODE::terminated;
                break;
        default:
                std::cout << "There's been an error - illegal intcode: " <<
                    c.operation << std::endl;
                exit(1);
                break;
    }

}

long SANTIAC::fetch_read(PARAMETER_MODE mode, long parameter_number) {
    long addr;
    long parameter = m_data[m_head + parameter_number];
    if (m_flags.debug_fetch) std::cout << "Parameter is ";
    if (mode == PARAMETER_MODE::immediate) {
        if (m_flags.debug_fetch) std::cout << "immediate " << parameter << " = " << parameter << std::endl;
        return parameter;
    } else if (mode == PARAMETER_MODE::relative) {
        if (m_flags.debug_fetch) std::cout << "relative " << parameter << " ( + " << m_base << ") = position " << m_base + parameter << " = " << m_data[m_base + parameter] << std::endl;
        return m_data[m_base + parameter];
    } else {
        if (m_flags.debug_fetch) std::cout << "position " << parameter << " = " << m_data[parameter] << std::endl;
        return m_data[parameter];
    }
}

long SANTIAC::fetch_write(PARAMETER_MODE mode, long parameter_number) {
    long addr;
    long parameter = m_data[m_head + parameter_number];
    if (m_flags.debug_fetch) std::cout << "Parameter is ";
    if (mode == PARAMETER_MODE::immediate) {
        if (m_flags.debug_fetch) std::cout << "immediate write. ERROR" << std::endl;
        return -1;
    } else if (mode == PARAMETER_MODE::relative) {
        if (m_flags.debug_fetch) std::cout << "relative addr " << parameter << " ( + " << m_base << ") = addr " << m_base + parameter << std::endl;
        return m_base + parameter;
    } else {
        if (m_flags.debug_fetch) std::cout << "position address " << parameter << " = addr " << parameter << std::endl;
        return parameter;
    }
}

void SANTIAC::op_add(PARAMETER_MODE m1, PARAMETER_MODE m2, PARAMETER_MODE m3) {
    // Fetch
    long arg1 = fetch_read(m1, 1);
    long arg2 = fetch_read(m2, 2);
    long arg3_addr = fetch_write(m3, 3);

    // Execute and write back
    m_data[arg3_addr] = arg1 + arg2;
    if (m_flags.debug) {
        std::cout << "Adding " << arg1 << " to " << arg2 << " and storing it in addr " << arg3_addr << std::endl;
        dump_data();
    }

    // Update instruction pointer
    m_head += 4;
}

void SANTIAC::op_mult(PARAMETER_MODE m1, PARAMETER_MODE m2, PARAMETER_MODE m3) {
    // Fetch
    long arg1 = fetch_read(m1, 1);
    long arg2 = fetch_read(m2, 2);
    long arg3_addr = fetch_write(m3, 3);

    // Execute and write back
    m_data[arg3_addr] = arg1 * arg2;
    if (m_flags.debug) {
        std::cout << "Multiplying " << arg1 << " to " << arg2 << " and storing it in addr " << arg3_addr << std::endl;
        std::cout << "We got " << arg1 * arg2 << std::endl;
        dump_data();
    }

    // Update instruction pointer
    m_head += 4;
}

// Start the copy of the amplifier controller software that will run on
// amplifier A. At its first input instruction, provide it the amplifier's phase
// setting, 3. At its second input instruction, provide it the input signal, 0.
// After some calculations, it will use an output instruction to indicate the
// amplifier's output signal.
void SANTIAC::op_read(PARAMETER_MODE m1) {
    // Fetch
    long arg1 = fetch_read(m1, 1);

    // Execute and write back
    long readMe = 0;
    if (m_config.unread) {
        if (m_flags.debug) {
            std::cout << "Reading in config: " << m_config.value << std::endl;
        }
        readMe = m_config.value;
        m_config.unread = false;
    } else if (!m_io.readMe.empty()) {
        readMe = m_io.readMe.front();
        m_io.readMe.pop();
        if (m_flags.debug) {
            std::cout << "Reading in data: " << readMe << std::endl;
        }
    } else {
        if (m_flags.output) {
            std::cout << "(" << m_config.value << "): ";
            std::cout << "Waiting on a read..." << std::endl;
        }
        if (m_flags.debug) {
            dump_data();
        }
        m_status = RUN_MODE::paused;
        return;
    }

    if (m1 == SANTIAC::PARAMETER_MODE::relative) {
        int addr = m_data[m_head + 1] + m_base;
        m_data[addr] = readMe;
        if (m_flags.debug) {
            std::cout << "Rel Reading in a " << readMe << " and writing it to addr " << addr << std::endl;
            dump_data();
        }
    } else {
        m_data[arg1] = readMe;
        if (m_flags.debug) {
            std::cout << "Reading in a " << readMe << " and writing it to addr " << arg1 << std::endl;
            dump_data();
        }
    }

    // Update instruction pointer
    m_head += 2;
}

void SANTIAC::op_print(PARAMETER_MODE m1) {
    // Fetch
    long arg1 = fetch_read(m1, 1);

    // Execute and write back
    long writeMe = arg1;
    if (m_flags.output) std::cout << "System printed " << writeMe << std::endl;
    m_io.prev_output = writeMe;
    if (m_flags.debug) {
        std::cout << "From addr " << arg1 << " we issue Print" << std::endl;
        dump_data();
    }

    // Update instruction pointer
    m_head += 2;
}

// Opcode 5 is jump-if-true: if the first parameter is non-zero, it sets the
// instruction pointer to the value from the second parameter. Otherwise, it
// does nothing.
void SANTIAC::op_jump_if_true(PARAMETER_MODE m1, PARAMETER_MODE m2) {
    // Fetch
    long arg1 = fetch_read(m1, 1);
    long arg2 = fetch_read(m2, 2);

    // Execute and write back
    if (m_flags.debug) {
        std::cout << "Arg1 is " << arg1 << " so we " << ((arg1 != 0)?("do"):("don't")) << " jump to arg2: " << arg2 << std::endl;
        dump_data();
    }
    if (arg1 != 0) {
        m_head = arg2;
    } else {
        // Update instruction pointer
        m_head += 3;
    }
}

// Opcode 6 is jump-if-false: if the first parameter is zero, it sets the
// instruction pointer to the value from the second parameter. Otherwise, it
// does nothing.
void SANTIAC::op_jump_if_false(PARAMETER_MODE m1, PARAMETER_MODE m2) {
    // Fetch
    long arg1 = fetch_read(m1, 1);
    long arg2 = fetch_read(m2, 2);

    // Execute and write back
    if (m_flags.debug) {
        std::cout << "Arg1 is " << arg1 << " so we " << ((arg1 == 0)?("do"):("don't")) << " jump to arg2: " << arg2 << std::endl;
        dump_data();
    }
    if (arg1 == 0) {
        m_head = arg2;
    } else {
        // Update instruction pointer
        m_head += 3;
    }
}

// Opcode 7 is less than: if the first parameter is less than the second
// parameter, it stores 1 in the position given by the third parameter.
// Otherwise, it stores 0.
void SANTIAC::op_is_less_than(PARAMETER_MODE m1, PARAMETER_MODE m2, PARAMETER_MODE m3) {
    // Fetch
    long arg1 = fetch_read(m1, 1);
    long arg2 = fetch_read(m2, 2);
    long arg3_addr = fetch_write(m3, 3);

    // Execute and write back
    long storeMe = (arg1 < arg2) ? 1 : 0;
    if (m_flags.debug) {
        std::cout << "(<) Arg1 is " << arg1 << ", Arg2 is " << arg2 << " so we write " << storeMe << " to addr " << arg3_addr << std::endl;
        dump_data();
    }
    m_data[arg3_addr] = storeMe;

    // Update instruction pointer
    m_head += 4;
}

// Opcode 8 is equals: if the first parameter is equal to the second parameter,
// it stores 1 in the position given by the third parameter. Otherwise, it
// stores 0.
void SANTIAC::op_is_equal(PARAMETER_MODE m1, PARAMETER_MODE m2, PARAMETER_MODE m3) {
    // Fetch
    long arg1 = fetch_read(m1, 1);
    long arg2 = fetch_read(m2, 2);
    long arg3_addr = fetch_write(m3, 3);

    // Execute and write back
    long storeMe = (arg1 == arg2) ? 1 : 0;
    if (m_flags.debug) {
        std::cout << "(==) Arg1 is " << arg1 << ", Arg2 is " << arg2 << " so we write " << storeMe << " to addr " << arg3_addr << std::endl;
        dump_data();
    }
    m_data[arg3_addr] = storeMe;

    // Update instruction pointer
    m_head += 4;
}

// Opcode 9 adjusts the relative base by the value of its only parameter. The
// relative base increases (or decreases, if the value is negative) by the value
// of the parameter.
void SANTIAC::op_adjust_relative_base(PARAMETER_MODE m1) {
    // Fetch
    long arg1 = fetch_read(m1, 1);

    // Execute and write back
    if (m_flags.debug) {
        std::cout << "Adjusting relative base (" << m_base << ") by " << arg1 << std::endl;
        dump_data();
    }
    m_base += arg1;

    // Update instruction pointer
    m_head += 2;
}

std::vector<std::string> SANTIAC::all_configs(long depth) {
    std::vector<std::string> answer;
    answer.push_back("");
    return all_configs_recursive(answer, depth - 1);
}

std::vector<std::string> SANTIAC::all_configs_recursive(
        std::vector<std::string>& current, long depth) {
    if (depth < 5) return current;
    std::vector<std::string> answer;
    std::string depth_str = std::to_string(depth);

    // "kern" is the space between letters in a font. If we have "01" and we
    // wish to insert "2", then we have 3 places to place the "2":
    // * "012"
    // * "021"
    // * "201"
    // ==> kern == 3
    long kern = current[0].size() + 1;
    for (long i = 0; i < kern; i++) {
        // For each kern column that we place our digit in, we must duplicate
        // the whole current vector
        for (auto str : current) {
            answer.push_back(str.insert(i, depth_str));
        }
    }

    return all_configs_recursive(answer, depth-1);
}

int main(int argc, char** argv) {
    SANTIAC myProgram(std::cin);
    myProgram.execute();
    return 0;
}
