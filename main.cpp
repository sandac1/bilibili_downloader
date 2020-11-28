#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include "bilibili_downloader.cpp"

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;

int main(int argc, char **argv)
{
    bilibili_downloader downloader;
    downloader.process(argc, argv);
}