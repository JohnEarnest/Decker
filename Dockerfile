
FROM nginx:latest

COPY ./js/build/lilt.js /usr/share/nginx/html/lilt.js
COPY ./js/build/decker.html /usr/share/nginx/html/index.html