#ifndef GOOGLEACESSACCOUNT_H
#define GOOGLEACESSACCOUNT_H

// To set up a Client ID and Secret, Visit:
//     https://console.developers.google.com/apis/dashboard

#define CLIENTID "124431808962-jj2udt9sgtrch9eo77iegmeq61edj296.apps.googleusercontent.com"
#define SECRET "_F4iFZG72iiKolwpqxNHdP8b"

// People API   - https://www.googleapis.com/auth/contacts
// Calendar API - https://www.googleapis.com/auth/calendar
// Contact API  - https://www.google.com/m8/feeds/
// My Details   - profile email

// Currently using Calendar and Contacts APIs
// Google are migrating to People API
// Need my details to retrieve current user name

#define SCOPE "https://www.googleapis.com/auth/calendar https://www.googleapis.com/auth/contacts https://www.google.com/m8/feeds/ profile email"

#endif // GOOGLEACESSACCOUNT_H
