// TODO make this actual class & stuff

    /** \name Web Server
     
        Simple web server that can be used to control the bridge. 
     */
    //@{

    static inline ESP8266WebServer server_{80};    

    static void initializeServer() {
        LOG("Initializing configuration web server...");
        if (!MDNS.begin("rckid-bridge"))
            LOG("  mDNS failed to initialize");
        server_.onNotFound(httpStaticHandler);
        //server_.on("/cmd", httpCommand);
        //server_.on("/status", httpStatus);
        //server_.on("/sdls", httpSDls);
        //server_.on("/sdrm", httpSDrm);
        //server_.on("/sd", httpSD);
        //server_.on("/sdUpload", HTTP_POST, httpSDUpload, httpSDUploadHandler);
        server_.begin();        
    }

    static void http404() {
        server_.send(404, "text/json","{ \"response\": 404, \"uri\": \"" + server_.uri() + "\" }");
    }

    static void httpStaticHandler() {
        http404();
    }

    static void loop() {
        MDNS.update();
        server_.handleClient();
    }
    
    //@}