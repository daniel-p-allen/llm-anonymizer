#ifndef GMAIL_CONNECTOR_H
#define GMAIL_CONNECTOR_H

#include <string>

class GmailConnector {
public:
    static std::string fetchEmails(int num_emails);
};

#endif
