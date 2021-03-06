/**
    XPIR-hash
    PIRServer.cpp
    Purpose: Parent class (abstract) that binds to each thread. Can have to modes of operation: Sequential or Pipeline (child classes)

    @author Joao Sa
    @version 1.0 18/01/17
*/

/**

                  PIRServer (*)
                      |
           ----------- -----------
           |                     |
    PIRServerSequential   PIRServerPipeline

*/

#include "PIRServer.hpp"

//***PRIVATE METHODS***//
/**
    Cleans DB folder to allow for another batch of files sent by the client.

    @param
    @return
*/
void PIRServer::removeDB(){
    int ret_val=std::system("exec rm -rf db/*");

    if (ret_val==1){
        cout << "Error performing system call" << endl;
    }
}

/**
    Reads the subtraction element (sub/request) from the client.

    @param size number of bytes of each element in the subtraction element 

    @return subtraction element
*/
vector<char*> PIRServer::readRequest(uint64_t size){
    int request_length = m_socket.readInt();

    vector<char*> request;
    double start_t = omp_get_wtime();
    for(int i=0;i<request_length;i++){
        char* request_part = new char[size];
        m_socket.readXBytes(size,(void*)request_part);
        request.push_back(request_part);
    }
    double end_t = omp_get_wtime();
    if(Constants::bandwith_limit!=0) m_socket.sleepForBytes(request_length*size+sizeof(int),end_t-start_t);
    return request;
}

/**
    Upload vcf data to be stored by the server

    @param
    @return
*/
void PIRServer::downloadData(){
    double start_t,end_t,total;
    /* Erase data in db folder */
    removeDB();

    uint64_t num_files=m_socket.readuInt64();
    for(uint64_t i=0;i<num_files;i++){
         try{
            total = 0;

            int len=m_socket.readInt();
            char* filename_c=m_socket.readChar(len);
            string filename(filename_c);

            int error=m_socket.readInt();
            if(error==1){
                removeDB();
                Error::error(error==1 || len==0,"Error uploading vcf files");
            }
            else if(error==2){
                i=-1;
                removeDB();
                delete[] filename_c;
                continue;
            }

            m_max_bytesize=m_socket.readInt();
            int num_entries=m_socket.readInt();
            for(int i=0;i<num_entries;i++){
                start_t = omp_get_wtime();
                char* recvBuff=m_socket.readChar(m_max_bytesize);
                end_t = omp_get_wtime();

                total+=end_t-start_t;

                //Create file where entries will be stored
                Tools::writeToBinFile("db/"+filename,recvBuff,m_max_bytesize);
                delete[] recvBuff;
            }
            delete[] filename_c;
            if(Constants::bandwith_limit!=0) m_socket.sleepForBytes(sizeof(int)+(filename.length()+1)*sizeof(char)+sizeof(int)+sizeof(int)+sizeof(int)+m_max_bytesize*num_entries,total);
        }catch (std::ios_base::failure &fail){
            Error::error(1,"Error writing DB files");
        }
    }
}