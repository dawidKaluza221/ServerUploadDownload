#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include <vector>
#include <map>
#include <string>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

struct NazwaFile {
        FILE* file;
        string nazwa;//download lub upload
        string Nazwapliku;//nazwa pliku do upload
        string NazwaDown;//nazwa pliku do download
};

int main()
{
    WSADATA wsaData;
    map < int, NazwaFile> files;
    vector<pollfd> pollFds;
    
    // inicjalizacja żądanej wersja biblioteki WinSock
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) { 
        std::cout << "error WSAStartup" << WSAGetLastError() <<std:: endl;
    }

    //utowrzenei socketa
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd == INVALID_SOCKET)
    {
        std::cout << "error socket" << WSAGetLastError()<<std::endl;
    }

    //inforamcje polaczeniowe

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa)); // czysczenie struktury
    sa.sin_family = AF_INET; 
    sa.sin_port = htons(2000);
    inet_pton(AF_INET, "0.0.0.0", &(sa.sin_addr));

    char ip4[INET_ADDRSTRLEN];
    int const buffer_size = 512;
    char buffer[buffer_size];
    char buffer1[buffer_size];
    int RecvSize;
    int SaveSize;

    if (bind(sockfd, (struct sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR)
    {
        std::cout << "blad binda " << WSAGetLastError() <<std::endl;
    }//laczenie socketa 
    if (listen(sockfd, 5) == SOCKET_ERROR)
    {
        std::cout << "Blad nasluchiwania" << WSAGetLastError() << std::endl;
    } //nasłuchuje za poalczenie


    //uzupelnie struktury
    
    struct pollfd pfds; 
    pfds.fd = sockfd;
    pfds.events = POLLRDNORM;
    //uzupelnienei struktury 

    pollFds.push_back(pfds);//dodanie struktury naszego serwera do listy

    sockaddr_in cli_addr;
    int nwm = sizeof(cli_addr);

    FILE* file;
    errno_t err;
    int newsockfd;
    string odczyt1;
    string filename;
    int reading_size;
    int sendsize;
    int wielkosc=0;
    err = 1;
    int zm=0;
    while (true)
    {
        zm = WSAPoll(pollFds.data(), pollFds.size(), 1500);
        if (zm == SOCKET_ERROR)
        {
            cout << "socekt status error" << WSAGetLastError() << endl;
        }
        else if (zm==0)
        {
            cout << "-" << endl;
        }
        else if (zm > 0)
        {
            if (pollFds[0].revents && POLLRDNORM )
            {
                newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &nwm);
                if (newsockfd == SOCKET_ERROR)
                {
                    cout<<(L"Error with accept", WSAGetLastError())<<endl;
                }
                else 
                {
                    inet_ntop(AF_INET, &(cli_addr.sin_addr), ip4, sizeof(ip4));
                    cout << "Connected to: " << ip4 << endl;

                    filename = string("adres") + string(ip4) + to_string(newsockfd) + string(".bin");
                    files[newsockfd].Nazwapliku = filename;
                    pfds.fd = newsockfd;
                    pollFds.push_back(pfds);
                }
            }
            else {
                for (int i = 1; i < pollFds.size(); i++) 
                {
                    int j;
                    if (pollFds[i].revents && POLLRDNORM)
                    {

                        int odebrane = 0;
                        int zapisane = 0;
                        
                            RecvSize = recv(pollFds[i].fd, buffer, buffer_size, 0);
                            odebrane =+ RecvSize;
                            if (RecvSize == SOCKET_ERROR)
                            {
                                if (err == 0)
                                fclose(files[pollFds[i].fd].file);
                                files.erase(pollFds[i].fd);
                                pollFds.erase(pollFds.begin() + i);
                                break;
                            }

                            else if (RecvSize == 0) 
                            {
                                if(err==0)
                                fclose(files[pollFds[i].fd].file);
                                files.erase(pollFds[i].fd);
                                pollFds.erase(pollFds.begin() + i);
                                break;
                            }
                            else if(RecvSize>0)
                            {
                                int a = 0;
                                int n = 0;
                                for ( j = 0;j <RecvSize;j++)
                                { 
                                    if (n == 0) 
                                    {
                                        files[pollFds[i].fd].nazwa += (buffer[j]);
                                        if (buffer[j+1] == '\n')//jesli znka nowej lini jest i nie bylo dotychcczasz : to znaczy reszte wiadmosci zapisujemy do buffer1
                                        {
                                            n = 1;
                                            j++;
                                        }
                                        else if (buffer[j+1] == ':') //jeseli przed znakiem nowej lini bedzie : to wtredy zaspizemy do buffor1 nazwe pliku ktory chemy wyslac 
                                        {
                                            n = 2;
                                            j++;
                                        }
                                        

                                    }
                                    else if (n == 1)
                                    {
                                        buffer1[a] = (buffer[j]);
                                        a++;
                                    }
                                    else if (n == 2) 
                                    {
                                        files[pollFds[i].fd].NazwaDown +=buffer[j];
                                        if (buffer[j + 1] == '\n')
                                        {
                                            break;
                                        }
                                        
                                    }
                                }
                                if (files[pollFds[i].fd].nazwa == "UPLOAD") 
                                {
                                    err = fopen_s(&files[pollFds[i].fd].file, files[pollFds[i].fd].Nazwapliku.c_str(), "wb");
                                    if (err != 0) {
                                        cout << ("Error opening file!") << endl;
                                    }
                                    
                                        SaveSize = fwrite(buffer1, sizeof(char), a, files[pollFds[i].fd].file);
                                        zapisane +=SaveSize;

                                        break;
                                }
                                else if (files[pollFds[i].fd].nazwa == "DOWNLOAD") 
                                {
                                    wielkosc = 0;
                                    odebrane = 0;
                                    reading_size = 0;
                                    sendsize = 0;
                                    err = fopen_s(&files[pollFds[i].fd].file, files[pollFds[i].fd].NazwaDown.c_str(), "rb");
                                    if (err != 0) {

                                        cout << ("Error opening file!") << endl;
                                       
                                        string sad;
                                        sad = "nie ma takiego pliku";
                                        strcpy_s(buffer1, sad.c_str());
                                        send(pollFds[i].fd, buffer1, 19, 0);
                                        break;
                                    }
                                    else 
                                    {
                                        if ((reading_size = fread(buffer1, sizeof(char), buffer_size, files[pollFds[i].fd].file)) < 0) {//odczyt z pliku tekstu do tablicy char o wielkosci buffer_size 
                                            cout << ("Error with reading data"); //fread zwraca nam wielkośćodczytanego pliku ktora nastepnie przypisujemy do reading_size

                                            break;
                                        }
                                        odebrane += reading_size;
                                        //cout << buf << endl;

                                        if ((sendsize = send(pollFds[i].fd, buffer1, reading_size, wielkosc)) < 0)//wysyłanie poprzez send tablicy buf o wielkosci reading_size(czyli tyle ile odczytaliśmy z tablicy)
                                        {//send zwraca liczbe odczytanych wilekosc 

                                            cout << ("Error with sending data");
                                            break;
                                        }
                                        wielkosc += sendsize;
                                        break;
                                    }
                                       
                                }
                                
                            }

                    }
                    else if (pollFds[i].revents && POLLERR)
                    {
                        fclose(files[pollFds[i].fd].file);
                        files.erase(pollFds[i].fd);
                        pollFds.erase(pollFds.begin() + i);
                    }
                    else if (pollFds[i].revents && POLLHUP) 
                    {
                        fclose(files[pollFds[i].fd].file);
                        files.erase(pollFds[i].fd);
                        pollFds.erase(pollFds.begin() + i);
                    }
                   
                }
                    

                       
                    
            }
            
            
            
        }
        
        
    }
    cout << "wychodzi za daleko " << endl;
    closesocket(sockfd);
    WSACleanup();
}
