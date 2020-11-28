#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <map>
#include <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;    // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http; // from <boost/beast/http.hpp>
using string = std::string;
class simple_client
{

public:
    simple_client() {}

    template<class response_body>
    http::response<response_body> get(std::string url, std::map<http::field, string> headers)
    {
        try
        {
            boost::asio::io_context ioc;
            tcp::resolver resolver{ioc};
            tcp::socket socket{ioc};
            size_t index = url.find_first_of("/");
            string host = url.substr(0, index);
            string target = url.substr(index);
            int version = 11;
            string port = "80";

            auto const results = resolver.resolve(host, port);
            boost::asio::connect(socket, results.begin(), results.end());
            http::request<http::string_body> req{http::verb::get, target, version};

            req.set(http::field::host,host);
            for (auto it = headers.begin(); it != headers.end(); it++)
            {
                req.set(it->first, it->second);
            }
            
            http::write(socket, req);
            boost::beast::flat_buffer buffer;
            http::response<response_body> res;
            http::read(socket, buffer, res);
            return res;
        }
        catch (std::exception const &e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    http::response<http::file_body> get(std::string url, std::map<http::field, string> headers,string file_path)
    {
        try
        {
            boost::asio::io_context ioc;
            tcp::resolver resolver{ioc};
            tcp::socket socket{ioc};
            size_t index = url.find_first_of("/");
            string host = url.substr(0, index);
            string target = url.substr(index);
            int version = 11;
            string port = "80";

            auto const results = resolver.resolve(host, port);
            boost::asio::connect(socket, results.begin(), results.end());
            http::request<http::string_body> req{http::verb::get, target, version};

            req.set(http::field::host,host);
            for (auto it = headers.begin(); it != headers.end(); it++)
            {
                req.set(it->first, it->second);
            }
            
            http::write(socket, req);
            boost::beast::flat_buffer buffer;

            http::response<http::file_body> res;
            boost::beast::error_code ec;

            res.body().open(file_path.c_str(),boost::beast::file_mode::write_existing,ec);
            
            
            res.prepare_payload();
            
            http::read(socket, buffer, res);
            res.body().close();
            return res;
        }
        catch (std::exception const &e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    std::map<string,string> head(std::string url, std::map<http::field, string> headers)
    {
        std::map<string,string> header_map;
        try
        {
            boost::asio::io_context ioc;
            tcp::resolver resolver{ioc};
            tcp::socket socket{ioc};
            size_t index = url.find_first_of("/");
            string host = url.substr(0, index);
            string target = url.substr(index);
            int version = 11;
            string port = "80";

            auto const results = resolver.resolve(host, port);
            boost::asio::connect(socket, results.begin(), results.end());
            http::request<http::string_body> req{http::verb::head, target, version};

            for (auto it = headers.begin(); it != headers.end(); it++)
            {
                req.set(it->first, it->second);
            }
            
            http::write(socket, req);
            // boost::beast::flat_buffer buffer;

            boost::asio::streambuf response;            
            boost::asio::read_until(socket,response,"\r\n");

            std::istream response_stream(&response);
            std::string http_version;
            unsigned int status_code;
            std::string status_message;

            response_stream>>http_version;
            response_stream>>status_code;
            std::getline(response_stream, status_message);
            if(status_code!=200)
            {
                throw "status code is not 200";
                return header_map;
            }
            std::string header;
            while (std::getline(response_stream, header) && header != "\r")
            {
                int index = header.find_first_of(":");
                header_map.insert(std::make_pair<string,string>(header.substr(0,index),header.substr(index+1)));
            }
            // for(auto it = header_map.begin();it!=header_map.end();++it)
            // {
            //     std::cout<<it->first<<":"<<it->second<<std::endl;
            // }
            return header_map;
        }
        catch (std::exception const &e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

};