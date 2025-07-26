
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cctype>
#include <filesystem>

const std::string audio_name = "base.ogg";

int to_number(const std::string& str)
{
    int num = 0;
    for (const auto &ch : str)
    {
        num *= 10;
        num += ch-'0';
    }
    return num;
}

std::string to_second(int milesecond)
{
    int second = milesecond / 1000;
    milesecond %= 1000;

    return std::to_string(second) + "." + std::to_string(milesecond);
}

int input_num(const int &l, const int r)
{
    std::string str;
    while (std::cin>>str)
    {
        bool err = 0;
        for(const auto &ch : str) if (!isdigit(ch)) err = 1;

        if (err)
        {
            std::cerr << "Invalid input" << std::endl;
            continue;
        }
        
        int num = to_number(str);

        if (!(l <= num && num <= r)) err = 1;

        if (err)
        {
            std::cerr << "Invalid input" << std::endl;
            continue;
        }

        return num;
    }
    return -1;
}

int _main_(void)
{
    std::string dir_name;
    std::cout << "Please enter the name of output folder:" << std::endl;
    std::cin >> dir_name;

    std::cout << "Please enter the difficulty of the chart (2FTR, 3BYD, 4ETR):" << std::endl;
    int difficulty = input_num(0, 4);

    if (difficulty == -1)
    {
        std::cerr << "Error: unable to read difficulty" << std::endl;
        return 1;
    }

    std::cout << "Please enter the start time and the end time (int terms of ms):" << std::endl;
    int start_time, end_time;
    std::cin >> start_time >> end_time;

    if (end_time <= start_time)
    {
        std::cerr << "Error: end time is before start time" << std::endl;
        return 2;
    }
    
    std::filesystem::path source_path = std::filesystem::current_path();
    std::filesystem::path destination_path = source_path / dir_name;
    try
    {
        if (!std::filesystem::exists(destination_path))
        {
            std::filesystem::create_directory(destination_path);
            std::cout << "Created destination path: " << destination_path << std::endl;
        }

        for (const auto &entry : std::filesystem::directory_iterator("./"))
        {
            if (entry.is_regular_file() && entry.path().has_extension())
            {
                std::string extension = entry.path().extension().string();
                for(auto &ch : extension) ch = tolower(ch);

                if (extension == ".jpg")
                {
                    std::filesystem::path destination_file_path = destination_path / entry.path().filename();
                    std::filesystem::copy(entry.path(), destination_file_path, std::filesystem::copy_options::overwrite_existing);
                    std::cout << "Copied: " << entry.path().filename() << std::endl;
                }
            }
        }
    }
    catch (const std::filesystem::filesystem_error &fserr)
    {
        std::cerr << fserr.what() << std::endl;
        return 3;
    }
    catch (const std::exception &err)
    {
        std::cerr << err.what() << std::endl;
        return 4;
    }

    std::filesystem::path source_audio_path = source_path / audio_name;
    if (!std::filesystem::exists(source_audio_path))
    {
        std::cerr << "Error: base.ogg does not exist" << std::endl;
        return 5;
    }

    std::string destination_audio_path = "./" + dir_name + "/" + audio_name;

    std::string command = "ffmpeg -i \"" + audio_name + "\" " +
                          "-ss " + to_second(start_time) + " " +
                          "-to " + to_second(end_time) + " " +
                          "-c copy \"" + destination_audio_path + "\"";

    std::cout << "Executing FFmpeg command: " << std::endl;
    std::cout << command << std::endl;
    
    int result = system(command.c_str());

    if (result == 0)
    {
        std::cout << "Successfully cut the audio" << std::endl;
    }
    else
    {
        std::cerr << "Error: unable to cut audio" << std::endl;
        return 6;
    }

    std::string chart_name = std::to_string(difficulty) + ".aff";
    std::filesystem::path source_chart_path = source_path / chart_name;
    std::filesystem::path destination_chart_path = destination_path / chart_name;

    if (!std::filesystem::exists(source_chart_path))
    {
        std::cerr << "Error: " << chart_name << " does not exist" << std::endl;
        return 7;
    }

    std::ifstream input_file(source_chart_path);
    std::ofstream output_file(destination_chart_path);

    if (!input_file.is_open())
    {
        std::cerr << "Error: unable to open " << source_chart_path << std::endl;
        return 8;
    }

    if (!output_file.is_open())
    {
        std::cerr << "Error: unable to open " << destination_chart_path << std::endl;
        return 9;
    }

    std::cout << "Ready to cut the chart: " << source_chart_path << std::endl;

    std::string line;
    std::vector<std::string> timing_group;
    bool is_in_timing_group = false;
    bool exist_timing_0 = false;
    bool exist_object_in_timing_group = false;
    int line_number = 1;

    while (std::getline(input_file, line))
    {
        std::string type;

        size_t split_position = 0;
        for ( ; split_position < line.size(); split_position++)
        {
            char ch = line[split_position];
            if (ch == ':' || ch == '(' || ch == '-' || ch == '{' || ch == '}') break;
            type.push_back(ch);
        }

        bool should_output = true;
        bool is_note = false;
        std::string output_line;
        std::string num_1, num_2;

        if (type == "arc" || type == "hold")
        {
            int object_start_time, object_end_time;

            bool meet_num = 0;
            size_t now_position = split_position + 1;
            for ( ; now_position < line.size(); now_position++)
            {
                if (isdigit(line[now_position]))
                {
                    meet_num = true;
                    num_1.push_back(line[now_position]);
                }
                else if (meet_num) break;
            }

            if (!meet_num)
            {
                std::cerr << "Error Line " << line_number << ": unable to read start time of " << type << std::endl;
                return 101;
            }

            object_start_time = to_number(num_1);

            meet_num = 0;
            for ( ; now_position < line.size(); now_position++)
            {
                if (isdigit(line[now_position]))
                {
                    meet_num = true;
                    num_2.push_back(line[now_position]);
                }
                else if (meet_num) break;
            }

            if (!meet_num)
            {
                std::cerr << "Error Line " << line_number << ": unable to read end time of " << type << std::endl;
                return 102;
            }

            object_end_time = to_number(num_2);

            if (object_start_time > end_time || object_end_time > end_time) should_output = false;

            int output_object_start_time = object_start_time - start_time;
            int output_object_end_time = object_end_time - start_time;

            if (output_object_start_time < 0 || output_object_end_time < 0) should_output = false;

            output_line = type + "(" + std::to_string(output_object_start_time) + "," + std::to_string(output_object_end_time);
            for ( ; now_position < line.size(); now_position++)
            {
                output_line.push_back(line[now_position]);
                if (type == "arc" && line[now_position] == ')') break;
            }

            if (type == "arc")
            {
                std::string num_arctap;
                for (now_position += 1; now_position < line.size(); now_position++)
                {
                    if (isdigit(line[now_position])) num_arctap.push_back(line[now_position]);
                    else
                    {
                        if (!num_arctap.empty())
                        {
                            int output_arctap_time = to_number(num_arctap) - start_time;
                            output_line += std::to_string(output_arctap_time);
                            num_arctap.clear();
                        }

                        output_line.push_back(line[now_position]);
                    }
                }
            }
        }
        else
        {
            bool get_num = true;

            if (type.empty())
            {
                if (line[split_position] != '(')
                {
                    output_line = line;
                    get_num = false;
                }
                else is_note = true;
            }
            else if (type == "AudioOffset" || type == "timinggroup")
            {
                if (type == "timinggroup") is_in_timing_group = true;
                output_line = line;
                get_num = false;
            }
            
            if (get_num) // scenecontrol timing note
            {
                int object_time;

                bool meet_num = 0;
                size_t now_position = split_position + 1;
                for ( ; now_position < line.size(); now_position++)
                {
                    if (isdigit(line[now_position]))
                    {
                        meet_num = true;
                        num_1.push_back(line[now_position]);
                    }
                    else if (meet_num) break;
                }

                if (!meet_num)
                {
                    std::cerr << "Error Line " << line_number << ": unable to read end time of " << (type.empty() ? "note" : type) << std::endl;
                    return 100;
                }

                object_time = to_number(num_1);

                int output_object_time = object_time - start_time;

                if (type == "timing" && output_object_time < 0) output_object_time = 0;

                if (output_object_time < 0 || object_time > end_time) should_output = false;

                output_line = type + "(" + std::to_string(output_object_time);
                for ( ; now_position < line.size(); now_position++) output_line.push_back(line[now_position]);
            }
        }

        if (is_in_timing_group)
        {
            if (exist_timing_0 && type == "timing" && num_1 == "0") should_output = 0;

            if (should_output)
            {
                timing_group.push_back(output_line);
                if (type == "timing" && num_1 == "0") exist_timing_0 = true;
                if (is_note || type == "arc" || type == "hold" || type == "scenecontrol") exist_object_in_timing_group = true;
            }
            if (line[split_position] == '}')
            {
                if (exist_object_in_timing_group) for (const std::string &now_line : timing_group) output_file << now_line << std::endl;
                timing_group.clear();
                is_in_timing_group = exist_timing_0 = exist_object_in_timing_group = false;
            }
        }
        else if (should_output) output_file << output_line << std::endl;

        line_number++;
    }

    input_file.close();
    output_file.close();

    std::cout << "Successfully cut the chart" << std::endl;

    return 0;
}

int main(void)
{
    bool quit = false;
    while(!quit)
    {
        int status = _main_();

        if (status)
        {
            system("pause");
            return status;
        }
        else
        {
            std::cout << "Again? (0:No / 1:Yes)" << std::endl;
            quit = !input_num(0, 1);
        }
    }

    std::cout << "Goodbye" << std::endl;

    system("pause");

    return 0;
}