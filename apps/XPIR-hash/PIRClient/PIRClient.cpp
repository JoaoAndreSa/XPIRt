#include "PIRClient.hpp"

//***PRIVATE METHODS***//
void PIRClient::createSocket(){
	m_listenFd = socket(AF_INET, SOCK_STREAM, 0);
	errorExit(m_listenFd<0,"Cannot open socket");
}

void PIRClient::connectToServer(){
	int check = connect(m_listenFd,(struct sockaddr *) &m_svrAdd, sizeof(m_svrAdd));
    errorExit(check<0,"Cannot connect!");
}

void PIRClient::getServerAddress(){
	m_server = gethostbyname(m_sname);
	errorExit(m_server==NULL,"Host does not exist");

	bzero((char *)&m_svrAdd, sizeof(m_svrAdd));
    m_svrAdd.sin_family = AF_INET;

    bcopy((char *)m_server->h_addr, (char *)&m_svrAdd.sin_addr.s_addr, m_server->h_length);
    m_svrAdd.sin_port = htons(m_portNo);
}

void PIRClient::sleepForBytes(unsigned int bytes) {
    uint64_t seconds=(bytes*8)/Constants::bandwith_limit;
    uint64_t nanoseconds=((((double)bytes*8.)/(double)Constants::bandwith_limit)-(double)seconds)*1000000000UL;

    struct timespec req={0},rem={0};
    req.tv_sec=seconds;
    req.tv_nsec=nanoseconds;

    nanosleep(&req,&rem);
}

// This assumes buffer is at least x bytes long,
// and that the socket is blocking.
void PIRClient::sendXBytes(uint64_t x, void* buffer){
    int bytesWrite = 0;
    while (bytesWrite < x){
        unsigned int result = write(m_listenFd, ((uint8_t*)buffer)+bytesWrite, x - bytesWrite); errorWriteSocket(result<0);
        bytesWrite += result;

        if(Constants::bandwith_limit!=0) sleepForBytes(result);
    }
}

void PIRClient::senduChar_s(unsigned char* c_str,int len){
    sendXBytes(len,(void*)c_str);
}

void PIRClient::sendInt(int integer){
	int v = htonl(integer);
    sendXBytes(sizeof(int),(void*)(&v));
}

void PIRClient::senduInt(unsigned int integer){
    unsigned int v = htonl(integer);
    sendXBytes(sizeof(unsigned int),(void*)(&v));
}

void PIRClient::senduInt32(uint32_t integer){
    uint32_t v = htonl(integer);
    sendXBytes(sizeof(uint32_t),(void*)(&v));
}

void PIRClient::senduInt64(uint64_t integer){
    uint64_t v = htonl(integer);
    sendXBytes(sizeof(uint64_t),(void*)(&v));
}

void PIRClient::sendVector_s(std::vector<char*> vector_c){
    uint64_t pos=0;
    senduInt64(m_xpir->getD());

    for(uint64_t j=1 ; j<=m_xpir->getD(); j++){
        uint32_t length=m_xpir->getQsize(j);
        senduInt32(length);

        senduInt(m_xpir->getN()[j-1]);
        for (uint64_t i=0; i<m_xpir->getN()[j-1]; i++){
            sendXBytes(length,(void*)vector_c[pos]);
            pos++;
        }
    }
}

// This assumes buffer is at least x bytes long,
// and that the socket is blocking.
void PIRClient::readXBytes(uint64_t x, void* buffer){
    int bytesRead = 0;
    while (bytesRead < x){
        int result = read(m_listenFd, ((uint8_t*)buffer)+bytesRead, x - bytesRead); errorReadSocket(result<0);
        bytesRead += result;
    }
}

uint64_t PIRClient::readuInt64(){
	uint64_t v=0;
    readXBytes(sizeof(uint64_t), (void*)(&v));

    return static_cast<uint64_t>(ntohl(v));
}

uint32_t PIRClient::readuInt32(){
    uint32_t v=0;
    readXBytes(sizeof(uint32_t), (void*)(&v));

    return static_cast<uint32_t>(ntohl(v));
}

std::vector<char*> PIRClient::readVector_s(){
    std::vector<char*> vector_s;

    uint64_t size=readuInt64();
    uint32_t message_length=readuInt32();

    double start = omp_get_wtime();
    for(uint64_t i=0; i<size;i++){
        char* buffer = new char[message_length];
        readXBytes(message_length,(void*)buffer);
        vector_s.push_back(buffer);
    }
    double end = omp_get_wtime();
    cout << "SimplePIR: Send reply took " << end-start << " seconds" << endl;
    return vector_s;
}

XPIRcSequential::REPLY PIRClient::readReply(){
	XPIRcSequential::REPLY reply;
	reply.nbRepliesGenerated=readuInt64();
	reply.aggregated_maxFileSize=readuInt64();
	reply.reply=readVector_s();
	return reply;
}

std::vector<char*> PIRClient::queryGeneration(uint64_t chosen_element){
    std::vector<char*> query;
    query=m_xpir->queryGeneration(chosen_element);

    return query;
}

char* PIRClient::replyExtraction(XPIRcSequential::REPLY reply){
	char* response;
    response=m_xpir->replyExtraction(reply);

    return response;
}

//return num_entries if SNPs are equal 
int PIRClient::compareSNPs(std::string t_curr, std::map<char,std::string> entry){
    istringstream curr(t_curr);
    std::vector<std::string> tokens_curr{istream_iterator<std::string>{curr},istream_iterator<std::string>{}};

    if( (atoi(tokens_curr[0].c_str()) == atoi(entry['c'].c_str())) && (atoi(tokens_curr[1].c_str()) == atoi(entry['p'].c_str())) && (tokens_curr[3]==entry['r'])  && (tokens_curr[4]==entry['a']) ){
        return 1;
    }else{
        return 0;
    }
}

int PIRClient::verifyParams(uint64_t num_entries, uint64_t d, uint64_t alpha, unsigned int* n){
    int total=alpha;

    for(int i=0;i<d;i++){
        total*=n[i];
    }

    if(total<num_entries) return 1;
    else return 0;
}

PIRParameters PIRClient::readParamsPIR(uint64_t num_entries){
    std::string line;
    PIRParameters params;

    ifstream f("../Constants/paramsPIR.txt");

    errorExit(f==NULL || f.is_open()==0,"Error reading file");
    if (f.is_open()){
        getline(f,line);
        params.d=atoi(line.c_str());

        getline(f,line);
        params.alpha=atoi(line.c_str());

        for(int i=0;i<4;i++){
            getline(f,line);
            params.n[i]=atoi(line.c_str());
        }
        errorExit(params.d<1 || params.d>4 || params.alpha<1 || params.alpha>num_entries || verifyParams(num_entries,params.d,params.alpha,params.n),"Wrong PIR parameters");

        getline(f,line);
        params.crypto_params=line;

        getline(f,line);
    }
    f.close();

    return params;
}

int PIRClient::readParamsSHA(){
    std::string line;

    ifstream f("../Constants/paramsSHA.txt");
    errorExit(f==NULL || f.is_open()==0,"Error reading file");
    if (f.is_open()){
        getline(f,line);
        int hex_pairs = atoi(line.c_str());

        errorExit(hex_pairs<=0,"Wrong SHA parameters");
        return hex_pairs;
    }
}

int PIRClient::symmetricEncrypt(unsigned char* ciphertext, std::string line){
    unsigned char ciphertext_noIV[1024];

    unsigned char *plaintext = new unsigned char[line.length()+1];
    memcpy((char*)plaintext,line.c_str(),line.length()+1);

    int ciphertexlen=m_cbc.encrypt(plaintext,strlen((char *)plaintext),ciphertext,ciphertext_noIV);

    delete[] plaintext;
    return ciphertexlen;
}

int PIRClient::symmetricDecrypt(unsigned char* decryptedtext, char* line){
    unsigned char ciphertext[1024];
    memcpy((char *)ciphertext,line,1024);

    int decryptedtextlen = m_cbc.decrypt(ciphertext,decryptedtext);
    decryptedtext[decryptedtextlen] = '\0';

    return decryptedtextlen;
}

void PIRClient::sendCiphertext(int ciphertexlen,unsigned char* ciphertext){
    sendInt(ciphertexlen);
    senduChar_s(ciphertext,ciphertexlen);
}

void PIRClient::sendPlaintext(int plaintexlen,string str){
    unsigned char *plaintext = new unsigned char[plaintexlen];
    memcpy((char*)plaintext,str.c_str(),plaintexlen);

    sendInt(plaintexlen);
    senduChar_s(plaintext,plaintexlen);
    delete[] plaintext;
}

uint64_t PIRClient::sendData(std::vector<std::string> data){
    uint64_t num_entries=0;
    for(uint64_t i=0; i<data.size();i++){

        if(data[i]!=""){
            //to send ciphertext
            unsigned char ciphertext[1024];
            int ciphertexlen = symmetricEncrypt(ciphertext,data[i]);
            sendCiphertext(ciphertexlen,ciphertext);
            //------ ### ------
        }else{
            std::string blank("0");
            sendPlaintext(blank.length(),blank.c_str());
        }

        //to send plaintext
        //sendPlaintext(data[i].length()+1,data[i]);
        //------ ### ------

        num_entries++;
    }
    sendInt(0);

    return num_entries;
}

uint64_t PIRClient::considerPacking(uint64_t pos){
    if(m_xpir->getAlpha()>1){
        return floor(static_cast<double>(pos)/m_xpir->getAlpha());
    }else{
        return pos;
    }
}

std::string PIRClient::extractCiphertext(char* response, uint64_t aggregated_entrySize, uint64_t pos){
    if(response[aggregated_entrySize*(pos%m_xpir->getAlpha())]=='0'){
        cout << "Reply: " << endl << endl;
        return "";
    }

    unsigned char decryptedtext[1024];
    int decryptedtextlen = symmetricDecrypt(decryptedtext,response+aggregated_entrySize*(pos%m_xpir->getAlpha()));

    std::string response_s(reinterpret_cast<char*>(decryptedtext));
    cout << "Reply: " << response_s << endl << endl;

    return response_s;
}

std::string PIRClient::extractPlaintext(char* response, uint64_t aggregated_entrySize, uint64_t pos){
    if(response+aggregated_entrySize*(pos%m_xpir->getAlpha())=='\0'){
        return "";
    }else{
        std::string response_s(response+aggregated_entrySize*(pos%m_xpir->getAlpha()));
        cout << "Reply: " << response_s << endl << endl;
        return response_s;
    }
}


//***PUBLIC METHODS***//
uint64_t PIRClient::uploadData(std::string filename){
	std::string line;

	ifstream f(filename);

    errorExit(f==NULL || f.is_open()==0,"Error reading file");

    //send the size of the vector to be stored
    std::vector<std::string>data(m_SHA_256->getSizeBits());

    if (f.is_open()){
        while(getline(f,line)){
            if(line[0]!='#'){
                int pos=m_SHA_256->hash(line);
                if(data[pos]!=""){
                    data[pos]+="->";
                }
                data[pos]+=line;
            }
        }
    }
    f.close();

	return sendData(data);
}

void PIRClient::initXPIR(uint64_t num_entries){
    m_xpir= new XPIRcSequential(readParamsPIR(num_entries),1,nullptr);
}

void PIRClient::initSHA256(){
    m_SHA_256= new SHA_256(readParamsSHA());
}

std::string PIRClient::searchQuery(uint64_t num_entries,std::map<char,std::string> entry){
    string query_str=entry['c']+" "+entry['p']+" # "+entry['r']+" "+entry['a'];
    uint64_t pos=m_SHA_256->hash(query_str);
    uint64_t pack_pos=considerPacking(m_SHA_256->hash(query_str));

    std::vector<char*> query=queryGeneration(pack_pos);
    sendVector_s(query);
    std::cout << "SimplePIR: Query sent" << "\n";

    XPIRcSequential::REPLY reply = readReply();
    char* response;
    response=replyExtraction(reply);
    std::string response_s;

    //if ciphertext
    response_s=extractCiphertext(response,reply.aggregated_maxFileSize,pos);
    //------ ### ------

    //if plaintext
    //response_s=extractPlaintext(response,reply.aggregated_maxFileSize,pos);
    //------ ### ------

    if(response_s!="") response_s = m_SHA_256->search(response_s,query_str);

    m_xpir->cleanQueryBuffer();
    cleanupVector(query);
    delete[] response;

    return response_s;
}

void PIRClient::cleanupVector(vector<char*> v){
    for(uint64_t i=0;i<v.size();i++){
        delete[] v[i];
    }
}

void PIRClient::cleanup(){
    delete m_SHA_256;
	delete m_xpir;
}

void PIRClient::setRTTStart(){
    m_RTT_start=omp_get_wtime();
}
double PIRClient::getRTTStart(){
    return m_RTT_start;
}

void PIRClient::setRTTStop(){
    m_RTT_stop=omp_get_wtime();
}
double PIRClient::getRTTStop(){
    return m_RTT_stop;
}