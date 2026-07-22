#include "email_logic.h"
#include "gmail_connector.h"
#include <iostream>

bool EmailLogic::processEmails(int num_emails) {
    std::string email_data = GmailConnector::fetchEmails(num_emails);

    // An empty result means the fetch failed; the connector has already
    // explained why.
    return !email_data.empty();
}
