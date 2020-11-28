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

    string vedio_name;

    string vedio_duration;

    string accept_description;

    string accept_quality;

    string download_quality;

public:
    bilibili_downloader() : download_quality("32")
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
                this->vedio_name = item.get<string>("part");
                this->vedio_duration = item.get<string>("duration");
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

    int download_vedio()
    {
        try
        {
            const string host_str = real_download_url.substr(0, real_download_url.find_first_of("/"));
            std::map<http::field, string> headers;
            headers.insert(std::make_pair<http::field, string>(http::field::user_agent, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/88.0.4295.0 Safari/537.36 Edg/88.0.680.1"));
            headers.insert(std::make_pair<http::field, string>(http::field::referer, "https://www.bilibili.com/"));
            headers.insert(std::make_pair<http::field, string>(http::field::host, host_str.c_str()));

            std::map<string, string> header_map = http_client.head(real_download_url, headers);
            string content_length_str = header_map["Content-Length"];

            std::stringstream sstream;
            sstream << content_length_str;
            int content_length = 0;
            sstream >> content_length;
            this->file_size = content_length;

            std::cout << "file_size:" << file_size / 1024 << "kb" << std::endl;

            int batch_size = 1024 * 1024 * 1;

            int batch_count = (content_length / batch_size) + ((content_length % batch_size) ? 1 : 0);

            int start_index = 0;
            int end_index = batch_size > content_length ? content_length : batch_size;

            string start_str;
            string end_str;

            //std::cout << "batch_count:" << batch_count << std::endl;

            std::ofstream ofs;
            ofs.open(this->file_path, std::ios_base::binary);
            std::cout<<"start download..."<<std::endl;
            for (int i = 0; i < batch_count; i++)
            {
                start_str = std::to_string(start_index);
                end_str = std::to_string(end_index-1);

                //headers.insert(std::make_pair<http::field,string>(http::field::range,"bytes="+start_str+"-"+end_str));
                headers[http::field::range] = "bytes=" + start_str + "-" + end_str;

                //http::response<http::file_body> res = http_client.get(real_download_url, headers,file_path);
                http::response<http::string_body> res = http_client.get<http::string_body>(real_download_url, headers);
                ofs << res.body();
                start_index = end_index;
                end_index = (start_index+batch_size>content_length)?content_length:(start_index+batch_size);

                std::cout << "download:" << ((i+1)*100.0f)/batch_count <<"%"<<"  "<<start_str<<"-"<<end_str<< "/"<<content_length<< std::endl;
            }
            if(ofs.is_open())
            {
             ofs.close();
            }
            std::cout<<"download complete"<<std::endl;
        }
        catch (string &err)
        {
            std::cerr<<err<<std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
        }

        //http::response<http::file_body> res = http_client.get(real_download_url, headers,"mm.flv");
    }

    int process(int argc, char **argv)
    {
        if(argc!=2)
        {
            std::cerr <<
                "Usage: bilibili-get <bvid>\n"<<
                "Example:\n"<<
                "   bilibili-get BV1BK4y1Z7SB"<<
            std::endl;
            return 0;
        }
        this->bvid = argv[1];
        try
        {

            this->file_path = this->bvid+".flv";
            cid = get_cid();
            real_download_url = get_download_url();

            std::cout << "name:" << this->vedio_name << std::endl;
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
            get_download_url();
            download_vedio();
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
        }
        //get cid
        //http::response<http::dynamic_body> res = http_client.get(download_url)
    }
};