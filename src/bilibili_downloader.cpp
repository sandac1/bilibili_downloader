#pragma once
#include "simple_client.cpp";
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <string>
#include <sstream>
#include <iostream>
using ptree = boost::property_tree::ptree;
class bilibili_downloader
{

    simple_client http_client;
    string cid;
    string bvid;

    string real_download_url;

    int file_size;

    string file_path;

    string video_name;

    string video_duration;

    string accept_description;

    string accept_quality;

    string download_quality;

    std::mutex write_file_mutex;

    string cookies_string;

    bool use_multi_thread;

public:
    bilibili_downloader() : download_quality("32"), use_multi_thread(false)
    {
    }

    string get_cid()
    {
        string url = "api.bilibili.com/x/player/pagelist?bvid=" + this->bvid + "&jsonp=jsonp";
        std::map<http::field, string> headers;
        headers.insert(std::make_pair<http::field, string>(http::field::user_agent, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/88.0.4295.0 Safari/537.36 Edg/88.0.680.1"));

        http::response<http::string_body> res = http_client.get<http::string_body>(url, headers);
        ptree pt;
        std::stringstream ss(res.body());
        //std::cout << res.body() << std::endl;
        try
        {
            boost::property_tree::read_json(ss, pt);

            ptree data_tree = pt.get_child("data");
            ptree item;
            for (auto it = data_tree.begin(); it != data_tree.end(); ++it)
            {
                item = it->second;
                this->video_name = item.get<string>("part");
                this->video_duration = item.get<string>("duration");
                return item.get<string>("cid");
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
        }
        return nullptr;
    }

    string get_download_url()
    {
        string url = "api.bilibili.com/x/player/playurl?bvid=" + this->bvid + "&cid=" + this->cid + "&qn=" + this->download_quality;
        std::map<http::field, string> headers;
        headers.insert(std::make_pair<http::field, string>(http::field::user_agent, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/88.0.4295.0 Safari/537.36 Edg/88.0.680.1"));
        //headers.insert(std::make_pair<http::field,string>(http::field::referer,"https://www.bilibili.com/"));
        http::response<http::string_body> res = http_client.get<http::string_body>(url, headers);

        ptree pt;

        std::stringstream ss(res.body());
        //std::cout<<res.body()<<std::endl;
        try
        {
            boost::property_tree::read_json(ss, pt);

            ptree data_tree = pt.get_child("data");
            ptree accept_description_tree = data_tree.get_child("accept_description");
            for (auto it = accept_description_tree.begin(); it != accept_description_tree.end(); it++)
            {
                this->accept_description += it->second.get_value<string>();
                this->accept_description += " ";
            }
            ptree accept_quality_tree = data_tree.get_child("accept_quality");
            // for(auto it = accept_quality_tree.begin();it!=accept_quality_tree.end();++it)
            // {
            //     this->accept_quality += it->second;
            //     this->accept_quality +=" ";
            // }
            //this->accept_description = pt.get<string>("accept_description");
            //this->accept_quality = pt.get<string>("accept_quality");
            ptree durl = data_tree.get_child("durl");
            ptree item;
            for (auto it = durl.begin(); it != durl.end(); ++it)
            {
                item = it->second;
                this->real_download_url = item.get<string>("url").substr(7);
                return item.get<string>("url").substr(7);
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
        }
        return nullptr;
    }

    int download_video()
    {
        try
        {
            const string host_str = real_download_url.substr(0, real_download_url.find_first_of("/"));
            std::map<http::field, string> headers;
            headers.insert(std::make_pair<http::field, string>(http::field::user_agent, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/88.0.4295.0 Safari/537.36 Edg/88.0.680.1"));
            headers.insert(std::make_pair<http::field, string>(http::field::referer, "https://www.bilibili.com/"));
            headers.insert(std::make_pair<http::field, string>(http::field::host, host_str.c_str()));

            if (!this->cookies_string.empty())
            {
                headers[http::field::cookie] = this->cookies_string;
            }

            std::map<string, string> header_map = http_client.head(real_download_url, headers);
            string content_length_str = header_map["Content-Length"];

            std::stringstream sstream;
            sstream << content_length_str;
            int content_length = 0;
            sstream >> content_length;
            this->file_size = content_length;

            std::cout << "file_size:" << file_size / 1024 << "kb" << std::endl;

            int batch_size = 1024 * 1024 * 4;

            int batch_count = (content_length / batch_size) + ((content_length % batch_size) ? 1 : 0);

            int start_index = 0;
            int end_index = batch_size > content_length ? content_length : batch_size;

            string start_str;
            string end_str;

            //std::cout << "batch_count:" << batch_count << std::endl;

            std::ofstream ofs;
            ofs.open(this->file_path, std::ios_base::binary);
            std::cout << "start download..." << std::endl;
            time_t start, end;
            time(&start);
            for (int i = 0; i < batch_count; i++)
            {
                start_str = std::to_string(start_index);
                end_str = std::to_string(end_index - 1);

                //headers.insert(std::make_pair<http::field,string>(http::field::range,"bytes="+start_str+"-"+end_str));
                headers[http::field::range] = "bytes=" + start_str + "-" + end_str;

                //http::response<http::file_body> res = http_client.get(real_download_url, headers,file_path);
                http::response<http::string_body> res = http_client.get<http::string_body>(real_download_url, headers);
                ofs << res.body();
                start_index = end_index;
                end_index = (start_index + batch_size > content_length) ? content_length : (start_index + batch_size);

                std::cout << "download:" << ((i + 1) * 100.0f) / batch_count << "%"
                          << "  " << start_str << "-" << end_str << "/" << content_length << std::endl;
            }
            if (ofs.is_open())
            {
                ofs.close();
            }
            time(&end);
            std::cout << "use time" << difftime(end, start) << std::endl;
            std::cout << "download complete" << std::endl;
        }
        catch (string &err)
        {
            std::cerr << err << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
        }

        //http::response<http::file_body> res = http_client.get(real_download_url, headers,"mm.flv");
    }

    int download_video_mt()
    {
        try
        {
            const string host_str = real_download_url.substr(0, real_download_url.find_first_of("/"));
            std::map<http::field, string> headers;
            headers.insert(std::make_pair<http::field, string>(http::field::user_agent, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/88.0.4295.0 Safari/537.36 Edg/88.0.680.1"));
            headers.insert(std::make_pair<http::field, string>(http::field::referer, "https://www.bilibili.com/"));
            headers.insert(std::make_pair<http::field, string>(http::field::host, host_str.c_str()));
            if (!this->cookies_string.empty())
            {
                headers[http::field::cookie] = this->cookies_string;
            }
            std::map<string, string> header_map = http_client.head(real_download_url, headers);
            string content_length_str = header_map["Content-Length"];

            std::stringstream sstream;
            sstream << content_length_str;
            int content_length = 0;
            sstream >> content_length;
            this->file_size = content_length;

            std::cout << "file_size:" << file_size / 1024 << "kb" << std::endl;
            const int thread_num = 4;
            std::thread download_threads[thread_num];

            int batch_size = 1024 * 1024 * 4;
            int batch_count = (content_length / batch_size) + ((content_length % batch_size) ? 1 : 0);
            int completed_batch_number = 0;

            int range_start;
            int range_end;

            int range_size_per_range = content_length / thread_num;

            std::ofstream ofs;
            ofs.open(this->file_path, std::ios_base::binary);
            std::cout << "start download..." << std::endl;

            for (int i = 0; i < thread_num; i++)
            {
                range_start = (range_size_per_range)*i;
                range_end = (range_start + (range_size_per_range)) < content_length ? (range_start + (range_size_per_range)) : content_length;
                if (range_start <= range_end)
                {
                    download_threads[i] = std::thread(&bilibili_downloader::download_job, this, range_start, range_end - 1, batch_size, std::ref(completed_batch_number), std::ref(ofs));
                }
            }

            for (int i = 0; i < thread_num; i++)
            {
                if (download_threads[i].joinable())
                {
                    download_threads[i].join();
                }
            }

            if (ofs.is_open())
            {
                ofs.close();
            }
            std::cout << "download complete" << std::endl;
        }
        catch (string &err)
        {
            std::cerr << err << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
        }
    }

    void download_job(int range_start, int range_end, int batch_size, int &complete_batch_number, std::ofstream &ofs)
    {

        const string host_str = real_download_url.substr(0, real_download_url.find_first_of("/"));
        std::map<http::field, string> headers;
        headers.insert(std::make_pair<http::field, string>(http::field::user_agent, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/88.0.4295.0 Safari/537.36 Edg/88.0.680.1"));
        headers.insert(std::make_pair<http::field, string>(http::field::referer, "https://www.bilibili.com/"));
        headers.insert(std::make_pair<http::field, string>(http::field::host, host_str.c_str()));

        if (!this->cookies_string.empty())
        {
            headers[http::field::cookie] = this->cookies_string;
        }

        string start_str;
        string end_str;
        int content_length = range_end - range_start + 1;
        int batch_count = (content_length / batch_size) + ((content_length % batch_size) ? 1 : 0);
        int start_index = range_start;
        int end_index = (start_index + batch_size > range_end + 1) ? range_end + 1 : (start_index + batch_size);
        std::cout << "[" << std::this_thread::get_id() << "]: start download" << std::endl;
        for (int i = 0; i < batch_count; i++)
        {
            start_str = std::to_string(start_index);
            end_str = std::to_string(end_index - 1);

            //headers.insert(std::make_pair<http::field,string>(http::field::range,"bytes="+start_str+"-"+end_str));
            headers[http::field::range] = "bytes=" + start_str + "-" + end_str;
            std::cout << "bytes=" << start_str << "-" << end_str << std::endl;

            //http::response<http::file_body> res = http_client.get(real_download_url, headers,file_path);
            time_t start, end;
            time(&start);
            http::response<http::string_body> res = http_client.get<http::string_body>(real_download_url, headers);
            time(&end);
            //std::cout<< "["<< std::this_thread::get_id()<<"]:"<<"get file use "<<difftime(end,start)<<std::endl;
            time(&start);
            {
                std::lock_guard<std::mutex>(this->write_file_mutex);
                ofs.seekp(range_start + i * batch_size, std::ios::beg);

                ofs << res.body();
                complete_batch_number++;
                std::cout << "[" << std::this_thread::get_id() << "]:"
                          << "download:" << ((i + 1) * 100.0f) / batch_count << "%"
                          << "  " << start_str << "-" << end_str << "/" << content_length << std::endl;
            }
            time(&end);
            //std::cout<< "["<< std::this_thread::get_id()<<"]:"<<"write file use "<<difftime(start,end)<<std::endl;

            start_index = end_index;
            end_index = (start_index + batch_size > range_end + 1) ? range_end + 1 : (start_index + batch_size);
        }
    }

    int set_cookies(string cookies_file_path)
    {
        try
        {
            std::ifstream ifs;
            ifs.open(cookies_file_path);
            if (ifs.is_open())
            {
                ifs >> this->cookies_string;
                std::cout << "use cookie: " << this->cookies_string << std::endl;
            }
            if (ifs.is_open())
            {
                ifs.close();
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
        }
    }

    int process(int argc, char **argv)
    {
        if (!(argc >= 2 && argc <= 5))
        {
        err:
            std::cerr << "Usage: bilibili-get <bvid> [-c <cookies_file_path>] [-mt]\n"
                      << "Example:\n"
                      << "   bilibili-get BV12234567890 -c cookies.txt -mt" << std::endl;
            return 0;
        }
        for (int i = 2; i < argc;)
        {
            string param = argv[i];
            if (param == "-c")
            {
                if (i == argc - 1)
                {
                    goto err;
                }
                string cookies_file_path = argv[3];
                set_cookies(cookies_file_path);
                i += 2;
            }
            else if (param == "-mt")
            {
                this->use_multi_thread = true;
                i += 1;
            }
            else
            {
                goto err;
            }
        }
        this->bvid = argv[1];
        try
        {

            this->file_path = this->bvid + ".flv";
            cid = get_cid();
            real_download_url = get_download_url();

            std::cout << "name:" << this->video_name << std::endl;
            std::cout << "description:" << this->accept_description << std::endl;

            string quality_choice = "";
            short quality_choice_index = 1;
            std::map<int, string> quality_map;
            if (this->accept_description.find("1080P") != this->accept_description.npos)
            {
                quality_map[quality_choice_index] = "80";
                quality_choice += "[" + std::to_string(quality_choice_index++) + "]1080P ";
            };
            if (this->accept_description.find("720P") != this->accept_description.npos)
            {
                quality_map[quality_choice_index] = "64";
                quality_choice += "[" + std::to_string(quality_choice_index++) + "]720P ";
            };
            if (this->accept_description.find("480P") != this->accept_description.npos)
            {
                quality_map[quality_choice_index] = "32";
                quality_choice += "[" + std::to_string(quality_choice_index++) + "]480P ";
            };
            if (this->accept_description.find("360P") != this->accept_description.npos)
            {
                quality_map[quality_choice_index] = "16";
                quality_choice += "[" + std::to_string(quality_choice_index++) + "]360P ";
            };
            std::cout << quality_choice << std::endl;
            std::cout << "please choose quality:" << std::endl;
            short input_quality_choice = 0;
        a:
            std::cin >> input_quality_choice;
            if (!(input_quality_choice <= quality_choice_index && input_quality_choice >= 0))
            {
                goto a;
            }
            this->download_quality = quality_map[input_quality_choice];
            std::cout << "choose quality:" << this->download_quality << std::endl;
            get_download_url();
            if (this->use_multi_thread)
            {
                download_video_mt();
            }
            else
            {
                download_video();
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
        }
        //get cid
        //http::response<http::dynamic_body> res = http_client.get(download_url)
    }
};