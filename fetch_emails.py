import base64
import json
import sys
from google.oauth2.credentials import Credentials
from google_auth_oauthlib.flow import InstalledAppFlow
from google.auth.transport.requests import Request
import os.path
from googleapiclient.discovery import build

SCOPES = ['https://www.googleapis.com/auth/gmail.readonly']

def fetch_emails(num_emails):
    creds = None
    if os.path.exists('token.json'):
        creds = Credentials.from_authorized_user_file('token.json')

    if not creds or not creds.valid:
        if creds and creds.expired and creds.refresh_token:
            creds.refresh(Request())
        else:
            flow = InstalledAppFlow.from_client_secrets_file(
                'secret.json', SCOPES)
            creds = flow.run_local_server(port=0)
            with open('token.json', 'w') as token:
                token.write(creds.to_json())

    service = build('gmail', 'v1', credentials=creds)

    results = service.users().messages().list(userId='me', maxResults=num_emails).execute()
    messages = results.get('messages', [])

    email_data = []
    for message in messages:
        msg = service.users().messages().get(userId='me', id=message['id']).execute()
        headers = msg['payload']['headers']

        from_email = next((header['value'] for header in headers if header['name'] == 'From'), "")
        to_email = next((header['value'] for header in headers if header['name'] == 'To'), "")
        subject = next((header['value'] for header in headers if header['name'] == 'Subject'), "")
        body = msg.get('snippet', "")

        email_data.append({
            "from": from_email,
            "to": to_email,
            "subject": subject,
            "content": body
        })
    # for message in messages:
    #     msg = service.users().messages().get(userId='me', id=message['id']).execute()
    #     headers = msg['payload']['headers']

    #     from_email = next((header['value'] for header in headers if header['name'] == 'from'), None)
    #     to_email = next((header['value'] for header in headers if header['name'] == 'to'), None)
    #     subject = next((header['value'] for header in headers if header['name'] == 'subject'), None)
    #     body = msg['snippet']

    #     email_data.append({
    #         "from": from_email,
    #         "to": to_email,
    #         "subject": subject,
    #         "content": body
    #     })

    with open('data.json', 'w') as f:
        json.dump(email_data, f, indent=4)

if __name__ == '__main__':
    num_emails = int(sys.argv[1]) if len(sys.argv) > 1 else 5
    fetch_emails(num_emails)



# def fetch_emails(num_emails=5, start_date=None, end_date=None):
#     creds = None
#     if os.path.exists('token.json'):
#         creds = Credentials.from_authorized_user_file('token.json')

#     if not creds or not creds.valid:
#         if creds and creds.expired and creds.refresh_token:
#             creds.refresh(Request())
#         else:
#             flow = InstalledAppFlow.from_client_secrets_file(
#                 'secret.json', SCOPES)
#             creds = flow.run_local_server(port=0)
#             with open('token.json', 'w') as token:
#                 token.write(creds.to_json())

#         service = build('gmail', 'v1', credentials=creds)

#     results = service.users().messages().list(userId='me', maxResults=num_emails).execute()
#     messages = results.get('messages', [])

#     email_data = []
#     for message in messages:
#         msg = service.users().messages().get(userId='me', id=message['id']).execute()
#         email_data.append(msg)

#     # Save the data to data.json
#     with open('data.json', 'w') as json_file:
#         json.dump(email_data, json_file, indent=4)

#     return "Data saved to data.json"

# if __name__ == '__main__':
#     num_emails = int(sys.argv[1]) if len(sys.argv) > 1 else 5
#     print(fetch_emails(num_emails))
