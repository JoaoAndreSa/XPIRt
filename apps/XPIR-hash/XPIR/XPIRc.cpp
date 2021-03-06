/**
    XPIR-hash
    XPIRc.cpp
    Purpose: Parent class (abstract) that encloses the XPIR library function calls

    @author Joao Sa
    @version 1.0 18/01/17
*/

/**

			    XPIRc (*)
			      |
	   ----------- -----------
	   |                     |
  XPIRcPipeline        XPIRcSequential

*/

#include "XPIRc.hpp"

//***PRIVATE METHODS***//
/**
    Deletes allocated 'tools'..

    @param
    @return
*/
void XPIRc::upperCleanup(){
    if(m_crypto!=nullptr){delete m_crypto;}
}


//***PUBLIC METHODS***//
uint64_t XPIRc::getAlpha(){
	return m_params.alpha;
}

uint64_t XPIRc::getD(){
	return m_params.d;
}

unsigned int* XPIRc::getN(){
	return m_params.n;
}

DBHandler* XPIRc::getDB(){
	return m_db;
}

HomomorphicCrypto* XPIRc::getCrypto(){
	return m_crypto;
}

/**
    GETTER for the query element size given a specific dimension (remember that each dimension can contain a different number of elements).

    @param d what dimension

    @return query_element_size (bytes)
*/
uint32_t XPIRc::getQsize(uint64_t d){
	return (uint32_t) m_crypto->getPublicParameters().getQuerySizeFromRecLvl(d)/GlobalConstant::kBitsPerByte;
}

/**
    GETTER for the reply element size.

    @param

    @return reply_element_size (bytes)
*/
uint32_t XPIRc::getRsize(uint64_t d){
	return (uint32_t) m_crypto->getPublicParameters().getCiphBitsizeFromRecLvl(d)/GlobalConstant::kBitsPerByte;
}


/**
    GETTER for the absorption size

    @param

    @return absorption size (bytes)
*/
uint32_t  XPIRc::getAbsorptionSize(uint64_t d){
    return (uint32_t) m_crypto->getPublicParameters().getAbsorptionBitsize(d)/GlobalConstant::kBitsPerByte;
}

vector<char*> XPIRc::getRequest(){
    return m_request;
}

PIRParameters XPIRc::getParams(){
    return m_params;
}

void XPIRc::setDB(DBHandler* db){
    m_db = db;
}

void XPIRc::setRequest(vector<char*> request){
    m_request=request;
}