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

std::string GmimePP::getHeader(const std::string &headerName) const
{
    std::string ret;
    const char *val = g_mime_object_get_header((GMimeObject *) m_gmessage, headerName.c_str());

    if (val == nullptr) {
            return "";
    }

    char* decodedValue = g_mime_utils_header_decode_text(val);
    ret.assign(decodedValue);
    g_free(decodedValue);
    return ret;
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

int GmimePP::addHeader(const std::string &header, const std::string &value) const
{
    int fd;
    
    if(remove(m_mailPath.c_str()) != 0) {
       return -1;
    }

    if ((fd = open(m_mailPath.c_str(), O_CREAT | O_RDWR, 0777)) == -1) {
        return -1;
    }	      

    char* encValue = g_mime_utils_header_encode_text(value.c_str());

    g_mime_object_append_header(GMIME_OBJECT (m_gmessage), header.c_str(), encValue);

    GMimeStream *gstream = g_mime_stream_fs_new (fd);
    if (g_mime_object_write_to_stream (GMIME_OBJECT (m_gmessage), gstream) < 0) {   
        g_free(encValue);
        g_object_unref (gstream);
        close(fd);
        return -1;
    }

    g_mime_stream_flush (gstream);
    g_object_unref (gstream);
    g_free(encValue);   
    close(fd);
    return 0;   
    
}


int GmimePP::setHeader(const std::string &header, const std::string &newValue) const
{
    int fd;

    if(remove(m_mailPath.c_str()) != 0) {
        return -1;
    }

    if ((fd = open(m_mailPath.c_str(), O_CREAT | O_RDWR, 0777)) == -1) {
        return -1;
    }	      

    char* encValue = g_mime_utils_header_encode_text(newValue.c_str());

    g_mime_object_set_header(GMIME_OBJECT (m_gmessage), header.c_str(), encValue);

    GMimeStream *gstream = g_mime_stream_fs_new (fd);
    if (g_mime_object_write_to_stream (GMIME_OBJECT (m_gmessage), gstream) < 0) {          
        g_free(encValue);
        g_object_unref (gstream);
        close(fd);
        return -1;
    }

    g_mime_stream_flush (gstream);
    g_object_unref (gstream);
    g_free(encValue);   
    close(fd);
    return 0;      
}

std::string GmimePP::getFromAdress() const
{
    InternetAddressList *list;
    const char *nameAndAdress = g_mime_message_get_sender(m_gmessage);
    list = internet_address_list_parse_string (nameAndAdress);
    
    if (list) {
        InternetAddress *address = internet_address_list_get_address (list, 0);
        if (address) {                        
            InternetAddressMailbox *mailbox;
            mailbox = INTERNET_ADDRESS_MAILBOX (address);
            
            return internet_address_mailbox_get_addr(mailbox);
        }
    }
  
    return "";   
}

int GmimePP::getRecipientsByType(GMimeRecipientType type, std::vector<SHeaderValue> &vRes) const
{
	const char *name = nullptr, *value = nullptr;

	InternetAddressList *rcptsList = g_mime_message_get_recipients (m_gmessage, type);
        
        if (rcptsList == nullptr)
            return 0;

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


int GmimePP::getAttachmentMimeParts(std::vector<GMimeObject *> &attachmentMimeParts) const
{
    GMimePartIter *it = g_mime_part_iter_new((GMimeObject *)m_gmessage);
    
    if (g_mime_part_iter_is_valid (it)) {
        while(g_mime_part_iter_next (it)) {
            GMimeObject *part = g_mime_part_iter_get_current (it);
            if (GMIME_IS_PART(part) ||GMIME_IS_MESSAGE_PART (part) || GMIME_IS_MESSAGE_PARTIAL (part)) {
                attachmentMimeParts.push_back(part);
            }           
        }
    }
    
    g_mime_part_iter_free(it);
    
    return 0;
}

int GmimePP::saveAttachments(const std::string &path) const
{
    int fd;
    std::vector<GMimeObject *> attachmentMimeParts;
    std::string filenameWithPath(path + "/");
    
    getAttachmentMimeParts(attachmentMimeParts);
    
    for (int i= 0; i < attachmentMimeParts.size(); i++) {
        GMimeDataWrapper *content;
        
	const char *filename;
	GMimeStream *stream;
        filename = g_mime_part_get_filename ((GMimePart *)attachmentMimeParts[i]);
        
        if (filename != nullptr)
            filenameWithPath.append(filename);
        else
            continue;
            
        if ((fd = open (filenameWithPath.c_str(), O_CREAT | O_WRONLY, 0755)) == -1)
            return -1;
	
	stream = g_mime_stream_fs_new (fd);
	
	content = g_mime_part_get_content_object ((GMimePart *)attachmentMimeParts[i]);
	g_mime_data_wrapper_write_to_stream (content, stream);
	g_mime_stream_flush (stream);
	
	g_object_unref (content);
	g_object_unref (stream);     
        close(fd);
    }
    
    return 0;
}

std::string GmimePP::getBody() const
{
    GMimeObject *gBody = g_mime_message_get_body (m_gmessage);
    char *bodyStr = g_mime_object_to_string(gBody);
    std::string messageBody = (bodyStr == NULL) ? "" : bodyStr;
    g_free (bodyStr);
    return messageBody;   
}


