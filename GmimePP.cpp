/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "GmimePP.hpp"
#include <iostream>


int GmimePP::init() 
{
    if (!isFileExist())
        return -1;
    
    g_mime_init(0);
    if ((m_fd = open(m_mailPath.c_str(), O_RDONLY)) == -1) {
            g_mime_shutdown (); 
            return -1;
    }
    
    GMimeStream *gstream = g_mime_stream_fs_new (m_fd);
    GMimeParser *gparser = g_mime_parser_new ();
    g_mime_parser_init_with_stream (gparser, gstream);
    m_gmessage = g_mime_parser_construct_message (gparser); 

    g_object_unref (gparser);
    g_object_unref (gstream);   
}

int GmimePP::getHeader(const std::string &headerName, std::string &value) const
{
    const char *val = g_mime_object_get_header((GMimeObject *) m_gmessage, headerName.c_str());

    if (val == nullptr) {
            return -1;
    }

    char* decodedValue = g_mime_utils_header_decode_text(val);
    value.assign(decodedValue);
    g_free(decodedValue);
    return 0;
}

int  GmimePP::getHeaders(std::vector<SHeaderValue> &vHeaderValuePairs) const
{
	const char *name = nullptr, *value = nullptr;
	GMimeHeaderIter it;

	GMimeHeaderList *headerList = g_mime_object_get_header_list(((GMimeObject *) m_gmessage));
	if (!headerList)
            return -1;

	if (!g_mime_header_list_get_iter (headerList, &it))
            return -1;

	do {
            if (!g_mime_header_iter_is_valid (&it))
                    break;
            name = g_mime_header_iter_get_name (&it);
            value = g_mime_header_iter_get_value (&it);

            SHeaderValue m {(name != NULL) ? name : "", (value != NULL) ? value : ""};
            vHeaderValuePairs.push_back(m);

	} while(g_mime_header_iter_next (&it));
        
	return 0;
}

int GmimePP::getRecipientsByType(GMimeRecipientType type, std::vector<SHeaderValue> &vRes) const
{
	const char *name = nullptr, *value = nullptr;

	InternetAddressList *rcptsList = g_mime_message_get_recipients (m_gmessage, type);

	internet_address_list_to_string (rcptsList, TRUE);
	int size = internet_address_list_length(rcptsList);

	for(int i = 0; i < size; i++) {
		InternetAddress *t = internet_address_list_get_address (rcptsList, i);
		name = internet_address_get_name (t);

		if (INTERNET_ADDRESS_IS_MAILBOX(t)) {
			value = internet_address_mailbox_get_addr(INTERNET_ADDRESS_MAILBOX(t));
		}
                if (value != nullptr)
                    vRes.push_back(SHeaderValue({name == nullptr ? "" : name, value}));
	}
        
	return 0;
}

int GmimePP::getAllRecipients(std::vector<SHeaderValue> &vRes) const
{
	const char *name = nullptr, *value = nullptr;

	InternetAddressList *rcptsList = g_mime_message_get_all_recipients (m_gmessage);

	if (rcptsList == nullptr)
		return -1;

	int size = internet_address_list_length(rcptsList);

	for(int i = 0; i < size; i++) {
		InternetAddress *t = internet_address_list_get_address (rcptsList, i);
		name = internet_address_get_name (t);
                
		if (INTERNET_ADDRESS_IS_MAILBOX(t)) {
			value = internet_address_mailbox_get_addr(INTERNET_ADDRESS_MAILBOX(t));
                        if (value != nullptr)
                            vRes.push_back(SHeaderValue({name == nullptr ? "" : name, value}));
		}
	}
	return 0;   
}

int main()
{
    using namespace std;
    GmimePP gpp("/home/iyasar/Desktop/test3.eml");
    std::vector<SHeaderValue> vHeaderValuePairs;
    
    gpp.init();
    
    gpp.getRecipientsByType(GMIME_RECIPIENT_TYPE_CC, vHeaderValuePairs);
    
    
    for (auto el : vHeaderValuePairs)
        cout << el.name << "->" << el.value << endl;
    
    return 0;
}