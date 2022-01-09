#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <WiFiAP.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <Adafruit_NeoPixel.h>

WebServer server(80);
//WiFiServer server(80);
//#define TEST
#define DBG_OUTPUT_PORT Serial

const char* ssid = "vikpvn_pasha";
const char* password = "";

int iregim = 0; 
int irange = 50;
unsigned long runTime;
bool changeregim = true;
bool changerange = false;
unsigned long lastTime = 0;
unsigned long timeThreshold = 0;
unsigned long timeDrop = 100;
int iraw = 22;
int icol = 42; // 10
size_t i, ind =0;
size_t checkfile = 0;
String strCon = "";
size_t stTicks = 0;
size_t slope = 16;
size_t countCom = 0;
size_t tekCom = 0;
size_t numCom = 0;
size_t uiMode = 0;
bool statCom = true;
bool btick = false;
size_t numShift = iraw;
//holds the current upload
File fsUploadFile;
size_t sizestrip = iraw*icol;
size_t sizebuf = iraw * icol * 3;
Adafruit_NeoPixel strip(sizestrip, 32, NEO_GRB + NEO_KHZ800);
size_t * reind = ( size_t *)malloc(sizestrip*sizeof(size_t));
unsigned char *bufF = ( unsigned char *)malloc(sizebuf);
unsigned char *bufE = ( unsigned char *)malloc(sizebuf);

// CRT таблица
const uint8_t CRTgammaPGM[256] PROGMEM = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3,
  4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 8,
  8, 8, 9, 9, 9, 10, 10, 10, 11, 11, 12, 12, 12, 13, 13, 14,
  14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22,
  23, 23, 24, 24, 25, 26, 26, 27, 28, 28, 29, 30, 30, 31, 32, 32,
  33, 34, 35, 35, 36, 37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 45,
  46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
  62, 63, 64, 65, 66, 68, 69, 70, 71, 72, 73, 74, 76, 77, 78, 79,
  81, 82, 83, 84, 86, 87, 88, 90, 91, 92, 94, 95, 96, 98, 99, 101,
  102, 103, 105, 106, 108, 109, 111, 112, 114, 115, 117, 118, 120, 122, 123, 125,
  126, 128, 130, 131, 133, 135, 136, 138, 140, 142, 143, 145, 147, 149, 150, 152,
  154, 156, 158, 160, 161, 163, 165, 167, 169, 171, 173, 175, 177, 179, 181, 183,
  185, 187, 189, 191, 193, 195, 197, 200, 202, 204, 206, 208, 210, 213, 215, 217,
  219, 221, 224, 226, 228, 231, 233, 235, 238, 240, 242, 245, 247, 250, 252, 255,
};

//format bytes
String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}

String getContentType(String filename) {
  if (server.hasArg("download")) return "application/octet-stream";
  else if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path) {
  DBG_OUTPUT_PORT.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
    if (SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void handleFileUpload() {
  if (server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();
   //DBG_OUTPUT_PORT.println("I am in handleFileUpload"); /////////////////////////////////
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) filename = "/" + filename;
    DBG_OUTPUT_PORT.print("handleFileUpload Name: "); DBG_OUTPUT_PORT.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    //DBG_OUTPUT_PORT.print("handleFileUpload Data: "); DBG_OUTPUT_PORT.println(upload.currentSize);
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile)
      fsUploadFile.close();
    DBG_OUTPUT_PORT.print("handleFileUpload Size: "); DBG_OUTPUT_PORT.println(upload.totalSize);
  }
}

void handleFileDelete() {
  if (server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileDelete: " + path);
  if (path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if (!SPIFFS.exists(path))
    return server.send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileCreate() {
  if (server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileCreate: " + path);
  if (path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if (SPIFFS.exists(path))
    return server.send(500, "text/plain", "FILE EXISTS");
  File file = SPIFFS.open(path, "w");
  if (file)
    file.close();
  else
    return server.send(500, "text/plain", "CREATE FAILED");
  server.send(200, "text/plain", "");
  path = String();
}

void handleGetParams(){
     if (server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");
  String regims = server.arg(0);
  String ranges = server.arg(1);

   #ifdef TEST
  String outParam = " regims = " + regims + " ranges = " + ranges;
  DBG_OUTPUT_PORT.println("outParam: " + outParam );
  #endif
  int regim =  regims.toInt();
  if(regim == 0){
      int range =  ranges.toInt();
    if( irange !=  range ){
      irange =  range;
      changerange = true;
    }
  }else{
      strCon = "";
      strCon += ranges;
  }
  if( iregim !=  regim ){
      iregim =  regim;
      changeregim = true;
      timeThreshold = 0;
      lastTime = 0;
  
  }
   
  server.send(200, "text/plain", regims);
 
  regims = String();
  ranges = String();
 
    
}

void returnFail(String msg) {
  server.send(500, "text/plain", msg + "\r\n");
}

void handleFileList() {
   if (!server.hasArg("dir")) {
    returnFail("BAD ARGS");
    return;
  }
  String path = server.arg("dir");
  if (path != "/" && !SPIFFS.exists((char *)path.c_str())) {
    returnFail("BAD PATH");
    return;
  }
  File dir = SPIFFS.open((char *)path.c_str());
  path = String();
  if (!dir.isDirectory()) {
    dir.close();
    returnFail("NOT DIR");
    return;
  }
  dir.rewindDirectory();

  String output = "[";
  for (int cnt = 0; true; ++cnt) {
    File entry = dir.openNextFile();
    if (!entry)
      break;

    if (cnt > 0)
      output += ',';

    output += "{\"type\":\"";
    output += (entry.isDirectory()) ? "dir" : "file";
    output += "\",\"name\":\"";
    // Ignore '/' prefix
    output += entry.name() + 1;
    output += "\"";
    output += "}";
    entry.close();
  }
  output += "]";
  server.send(200, "text/json", output);
  dir.close();
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  DBG_OUTPUT_PORT.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    DBG_OUTPUT_PORT.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    DBG_OUTPUT_PORT.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      DBG_OUTPUT_PORT.print("  DIR : ");
      DBG_OUTPUT_PORT.println(file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      DBG_OUTPUT_PORT.print("  FILE: ");
      DBG_OUTPUT_PORT.print(file.name());
      DBG_OUTPUT_PORT.print("  SIZE: ");
      DBG_OUTPUT_PORT.println(file.size());
    }
    file = root.openNextFile();
  }
}


//////////////////////////////////////////////////////
void printStr(String namestr, String str){
    size_t lstr = str.length();
    size_t in = 0;
     DBG_OUTPUT_PORT.print("\n");
     DBG_OUTPUT_PORT.println(namestr);
    for(in = 0; in < lstr; in++){
        DBG_OUTPUT_PORT.printf("%c", str[in]);
    }
    DBG_OUTPUT_PORT.print("\n");
    
}
void switchOff(){
    
   strip.clear(); 
   DBG_OUTPUT_PORT.printf("\n strip.clear() regim = 0");
  for( ind = 0; ind < sizebuf; ind++){
       
       bufF[ind] = 0;
    
    } 
  strip.show(); 
  DBG_OUTPUT_PORT.printf("\n strip.show() regim = 0");
   changeregim = false;
   
}
bool switchOn(String filen, unsigned char * buf){
    
    size_t ind = 0;
    size_t i, j = 0;
    int raw =0;
    int col =0;
    unsigned char data = 0;
    unsigned char dath = 0;
    unsigned char datl = 0;
    if(!filen.startsWith("/")) filen = "/" + filen;
    
    
    File file = SPIFFS.open(filen, "r");
    if(!file){
        DBG_OUTPUT_PORT.printf("\n file did not open name = %s ", filen);
    }
    size_t szfile = file.size();
    char *by = ( char *)malloc(szfile);
    
   size_t rsize = file.readBytes(by, szfile);
    #ifdef TEST
    DBG_OUTPUT_PORT.println(file.name());
    DBG_OUTPUT_PORT.printf("\n ");
    DBG_OUTPUT_PORT.printf("\n rsize = %d ", rsize);
    DBG_OUTPUT_PORT.printf("\n");
    
     for( ind = 0; ind < szfile; ind++){ 

      DBG_OUTPUT_PORT.printf("%c", by[ind]);
        
   }
    #endif
   
    char * search_char = (char*) memchr(by, '[', strlen(by));
    ind = search_char - by + 1;
    
    raw =  (by[ind] - 48) * 10 + (by[ind + 1] - 48);
    search_char = (char*) memchr(by, ':', strlen(by));
    ind = search_char - by + 1;
    
    col =  (by[ind] - 48) * 10 + (by[ind + 1] - 48);
    if(col != icol || raw != iraw){
        DBG_OUTPUT_PORT.printf("The configuration of the file does not match the system!");
        file.close();
        free(by);
        changeregim = false;
        return(false);
        
    }
    
    sizebuf = iraw * icol * 3;
    #ifdef TEST
    DBG_OUTPUT_PORT.printf("\n");
    DBG_OUTPUT_PORT.printf(" sizebuf = %d", sizebuf);
    #endif
     ///////////////////////////////////////////////////////////////////
    if(filen.endsWith(".l1p")){
        
         search_char = (char*) memchr(by, '#', strlen(by));
         ind = search_char - by + 1;
    
         for( i = 0; i < sizebuf; i+=3){
        
            for( j = 0; j < 3; j++){
            
              dath =  by[ind] - 48;
              if(dath > 9) dath = by[ind] - 87;
              datl =  by[ind+1] - 48;
              if(datl > 9) datl = by[ind+1] - 87;
              if(dath> 15) dath = 0;
              if(datl> 15) datl = 0;
              data =  dath * 16 + datl;
              ind += 2;
              buf[i+j] = data;
            }
        
          ind += 2;
        
        }
        
    }else{
         unsigned char bcol[16][3];
         size_t countcol = 0;
         search_char = (char*) memchr(by, '#', strlen(by));
         ind = search_char - by + 1;
         
        while(by[ind] != '|' && countcol < 16){
             
             for( j = 0; j < 3; j++){
                 
                 dath =  by[ind] - 48;
                 if(dath > 9) dath = by[ind] - 87;
                 datl =  by[ind+1] - 48;
                 if(datl > 9) datl = by[ind+1] - 87;
                 if(dath> 15) dath = 0;
                if(datl> 15) datl = 0;
                data =  dath * 16 + datl;
                ind += 2;
               bcol[countcol][j] = data;
             }
             
             ind += 1;
             countcol++;
        } 
          search_char = (char*) memchr(by, '|', strlen(by));
          ind = search_char - by + 1;
          
          for( i = 0; i < sizebuf; i+=3){
        
             for( j = 0; j < 3; j++){
                 
                dath =  by[ind] - 48;
                if(dath > 9) dath = by[ind] - 87;
                if(dath> 15) dath = 0;
                buf[i+j] = bcol[dath][j];
             }
        
             ind++;
        
         }
    }
   
    #ifdef TEST
    DBG_OUTPUT_PORT.printf("\n");
    for( ind = 0; ind < sizebuf; ind++){ 

      DBG_OUTPUT_PORT.printf("%d[%d] ,", buf[ind], ind);
        
   }
   #endif
   file.close();
   free(by);
   
   return(true);

}
void loadConFile(){
    
    
    int raw =0;
    int col =0;
    
    File file = SPIFFS.open("/do.txt", "r");
    if(!file){
        DBG_OUTPUT_PORT.printf("\n did not open ConFile");
        return;
    }
    size_t szfile = file.size();
    char *by = ( char *)malloc(szfile);
    
   size_t rsize = file.readBytes(by, szfile);
   strCon = "";
   for( ind = 0; ind < rsize; ind ++){
       strCon += by[ind];
   }
   
   
    #ifdef TEST
    DBG_OUTPUT_PORT.println(file.name());
    DBG_OUTPUT_PORT.printf("\n");
    DBG_OUTPUT_PORT.printf("\n rsize = %d ", rsize);
    DBG_OUTPUT_PORT.printf("\n");
    DBG_OUTPUT_PORT.printf("\n rsize strCon = %d ", strCon.length());
    printStr("strCon", strCon);
    
    #endif
   
   free(by);
   
    
}
size_t GetCountCom(String strC){
    size_t count =0;
    size_t sizes = strC.length();
    for(ind = 0 ; ind < sizes; ind++){
        if(strC[ind] == ';'){
            count++;
        }
    }
    return(count);
}
void setregFile(){
    
    
    
    
        loadConFile();
        
        if(strCon != ""){
            
            countCom = GetCountCom(strCon);
            
            if(countCom > 0){
                tekCom =0;
                StartCom();
               
            }
            
            
        }
    
        
   
    
    
}
void setregTerminal(){
    DBG_OUTPUT_PORT.println("setregTerminal strCon = " + strCon );
    
        if(strCon != ""){
            
            countCom = GetCountCom(strCon);
            
            if(countCom > 0){
                tekCom = 0;
                StartCom();
                
            }
            
            
        }
    
}
void StartCom(){
    
    size_t count =0;
    size_t counsim =0;
    size_t sizes = strCon.length();
    String stemp = "";
    String fileName = "";
    String stime = "";
    size_t smode = 0;
    unsigned char arcolor[3];
    uiMode = 0;
    
    for(ind = 0 ; ind < sizes; ind++){
        
        
        
        if(strCon[ind] == ';'){
           
            if(count == tekCom){
                
                for(i = 0; i < counsim; i++ ){
                    
                    stemp += strCon[ ind - counsim + i ];
                }
                break;
                
            }
            count++;
            counsim =0;
        }else{
             counsim++;
        }
         
       //  counsim++;
    }
    DBG_OUTPUT_PORT.printf("\n");
    DBG_OUTPUT_PORT.printf(" Comand: %d ",tekCom);
    DBG_OUTPUT_PORT.printf(" size stemp: %d ", stemp.length());
    printStr("stemp = ", stemp);
    
    sizes = stemp.length();
    ind = 0;
    
        
        switch(stemp[ind]){
            
          case 'l':
          case 'L':
         if(stemp[ind+1] == 'n'){
             
              smode = 1;
          }
          if(stemp[ind+1] == 'y'){
              uiMode = 1;
              smode = 1;
          }
          if(stemp[ind+1] == 'x'){
              uiMode = 2;
              smode = 1;
          }
          if(stemp[ind + smode + 1] == 'f'){
              numCom = 1;
              break;
          }
          if(stemp[ind + smode + 1] == 'c'){ 
              numCom = 2;
              break;
          }
          break;
          case 'p':
          case 'P':
          if(stemp[ind+1] == 'f'){
              numCom = 3;
             
          }else{
              numCom = 4;
          }
          break;
          case 'r':
          case 'R':
          if(stemp[ind+1] == 'l'){
              uiMode = 1;
              smode = 1;
          }
          if(stemp[ind+1] == 'r'){
              uiMode = 2;
              smode = 1;
          }
          if(stemp[ind+1] == 't'){
              uiMode = 3;
              smode = 1;
          }
          if(stemp[ind+1] == 'b'){
              uiMode = 4;
              smode = 1;
          }
          if(stemp[ind + smode + 1] == 'f'){
              numCom = 5;
              break;
          }
          if(stemp[ind + smode + 1] == 'c'){ 
              numCom = 6;
              break;
          }
          break;
         // default:
           
     
            
        }// swith
        
    for(ind = 2 + smode; ind < sizes; ind++){
        
        
        if(stemp[ind] == '('){
            stime ="";
        }else{
            if(stemp[ind] == ')'){
             break;
        }else{
            
            stime += stemp[ind];
            
        }
        }
        
        
    }
    if(numCom == 1 || numCom == 2){
        
        stTicks = 2;
        timeDrop = (unsigned long)stime.toInt()/stTicks;
        
    } 
    if(numCom == 3 || numCom == 4){
        
        stTicks = slope;
        timeDrop = (unsigned long)stime.toInt()/stTicks;
        
    } 
    if(numCom == 5 || numCom == 6){
        
        if(uiMode < 3){
            stTicks = iraw;
            numShift = iraw;
        }else{
            stTicks = icol;
            numShift = icol;
        }
        timeDrop = (unsigned long)stime.toInt()/stTicks;
        
        
    } 
   // stTicks = (size_t)((unsigned long)atol(stime)/timeDrop); 
    DBG_OUTPUT_PORT.print("\n");
    DBG_OUTPUT_PORT.printf("stTicks = %d", stTicks);
    DBG_OUTPUT_PORT.printf("numCom = %d", numCom);
    DBG_OUTPUT_PORT.print("\n");
    
    ind++;
    if(numCom == 1 || numCom == 3 || numCom == 5 ){
        while( ind < sizes){
            
            if( stemp[ind] != ';' ) fileName += stemp[ind];
            ind++;
        }
        printStr("fileName = ", fileName); 
        
    }else{
        
        count =0;
        String str = "";
        
        for(i = ind; i < sizes; i++ ){
            
            
            
            if( stemp[i] == ',' || stemp[i] == ';' ){
                
               arcolor[count] = (unsigned char)str.toInt();
               count++;
               str = "";
                
            }else{
                
                str += stemp[i];
            }
            
        }
        if(str != "" && count == 2) arcolor[count] = (unsigned char)str.toInt();
        
    }
    
    if(numCom == 1){
        
        if(!switchOn(fileName, bufF)){
            
            DBG_OUTPUT_PORT.printf(" Error!!! Command = %d", numCom); 
        }
        if(uiMode == 1){
            char bb;
            int index,index2;
            int middle  = iraw/2;
            for(int im = 0; im < middle; im++){
                    
                for(int in = 0; in< icol; in++){
                    
                    index = (im*icol + in) *3;
                    index2 = ((iraw - im -1) * icol + in)*3;
                    bb = bufF[index];
                    bufF[index]=bufF[index2];
                    bufF[index2] = bb;
                    bb = bufF[index + 1];
                    bufF[index + 1]=bufF[index2 + 1];
                    bufF[index2 + 1] = bb;
                    bb = bufF[index + 2];
                    bufF[index + 2]=bufF[index2 + 2];
                    bufF[index2 + 2] = bb;
                }
            }
        }
        if(uiMode == 2){
            char bb;
            int index,index2;
            int middle  = icol/2;
            for(int im = 0; im < middle; im++){
                    
                for(int in = 0; in< iraw; in++){
                    
                    index = (im  + in * icol ) *3;
                    index2 = ((icol - im -1) + in * icol)*3;
                    bb = bufF[index];
                    bufF[index]=bufF[index2];
                    bufF[index2] = bb;
                    bb = bufF[index + 1];
                    bufF[index + 1]=bufF[index2 + 1];
                    bufF[index2 + 1] = bb;
                    bb = bufF[index + 2];
                    bufF[index + 2]=bufF[index2 + 2];
                    bufF[index2 + 2] = bb;
                }
            }
        }
        
    }
    if(numCom == 3){
        
        if(!switchOn(fileName, bufE)){
            
             DBG_OUTPUT_PORT.printf(" Error!!! Command = %d", numCom); 
        }
        
    }
     if(numCom == 5){
       
        if(!switchOn(fileName, bufE)){
            
             DBG_OUTPUT_PORT.printf(" Error!!! Command = %d", numCom); 
        }
        
    }
     if(numCom == 2){
       
        colorToBuf(arcolor, bufF);
        
    }
    if(numCom == 4){
        
        colorToBuf(arcolor, bufE);
        
    }
    if(numCom == 6){
        
        colorToBuf(arcolor, bufE);
        
    }
    showBuf();
    setTime();
}

void setTime(){
    
    timeThreshold = timeDrop;
    lastTime = 0;
    
}

void showBuf(){
    
    size_t datt =0;
   unsigned char col0, col1, col2 = 0;
   sizestrip = iraw*icol;
  
   for( ind = 0; ind < sizestrip; ind++){
       
       i = reind[ind]*3;
       datt = (irange * bufF[i])/100;
       col0 = (unsigned char) datt;
       datt = (irange * bufF[i + 1])/100;
       col1 = (unsigned char) datt;
       datt = (irange * bufF[i + 2])/100;
       col2 = (unsigned char) datt;
       
       strip.setPixelColor(ind, pgm_read_byte(&(CRTgammaPGM[col0])),  pgm_read_byte(&(CRTgammaPGM[col1])), pgm_read_byte(&(CRTgammaPGM[col2])));
       //strip.setPixelColor(ind, col0,  col1, col2);
       
   }
    
   strip.show(); 
  
}
void colorToBuf(unsigned char * arcol, unsigned char * buf){
    DBG_OUTPUT_PORT.println(" colorToBuf!!!\n");
    DBG_OUTPUT_PORT.printf(" numCom = %d\n", numCom);
    
    size_t j = 0;
    
    for( ind = 0; ind < sizebuf; ind++){
        
        buf[ind] = arcol[j];
        if(j < 2) j++;
        else j=0;
    }
    
}
void setTick(){
   // DBG_OUTPUT_PORT.println("!!!");
    
    if(stTicks != 0){
        
        if(numCom == 1 || numCom == 2){
        
            setTime();
        
        }
        if(numCom == 3 || numCom == 4 ){
            
            size_t dd = 0;
        
            for( ind = 0; ind < sizebuf; ind++){
                
                if(bufE[ind] > bufF[ind]){
                   
                    
                    dd = bufE[ind] - bufF[ind];
                    
                    if(dd >= slope){
                        
                        bufF[ind] += slope;
                        
                    }else{
                        
                         bufF[ind] = bufE[ind];
                        
                    }
                       
                        
                    
                }
                if(bufE[ind] < bufF[ind]){
                    
                   
                    
                    dd = bufF[ind] - bufE[ind];
                    
                    if(dd >= slope){
                        
                        bufF[ind] -= slope;
                        
                    }else{
                        
                         bufF[ind] = bufE[ind];
                        
                    }
                    
                   
                }
                
                    
            }
             showBuf();
             setTime();
        }
        if(numCom == 5 || numCom == 6){
            
            numShift--;
            
            if(uiMode == 0 || uiMode == 1){
                
                    size_t colshift = icol * 3;
                    
                for( ind = 0; ind < numShift; ind++){
                
                 for( i = 0; i < colshift; i++){
                    
                     bufF[ind * colshift + i] = bufF[(ind + 1) * colshift + i];
                    
                     }
                
                 }// for ind
                for( ind = numShift; ind < iraw; ind++){
                
                 for( i = 0; i < colshift; i++){
                    
                     bufF[ind * colshift + i] = bufE[(ind - numShift) * colshift + i];
                     
                    }
                
                }// for ind
            }
            
            if(uiMode == 2){
                
                    size_t colshift = icol * 3;
                    
                for( int ind = iraw-1; ind >= 1; ind--){
                
                 for( i = 0; i < colshift; i++){
                    
                     bufF[ind * colshift + i] = bufF[(ind - 1) * colshift + i];
                     
                    }
                
                }// for ind
                    
                ind = numShift;
                
                 for( i = 0; i < colshift; i++){
                    
                     bufF[ i] = bufE[ ind  * colshift + i];
                    
                     }
                
                 
                
            }
            if(uiMode == 3){
                
                    size_t colshift = icol * 3;
                    
                    size_t tind =0;
                    size_t tind2 =0;
                    
                for( ind = 0; ind < icol; ind++){
                
                 for( i = 0; i < iraw; i++){
                     
                     tind =  (ind * 3 )  + (colshift * i);
                     tind2 = (ind + 1) * 3 + (colshift * i);
                     bufF[tind ] = bufF[tind2];
                     bufF[tind + 1] = bufF[tind2 + 1];
                     bufF[tind + 2] = bufF[tind2 + 2];
                    }
                
                }// for ind
                    
                ind = icol - numShift - 1;
                
                 for( i = 0; i < iraw; i++){
                    
                     tind = ((icol - 1) * 3)  + (colshift * i);
                     tind2 = (ind * 3) + (colshift * i);
                     bufF[tind ] = bufE[tind2];
                     bufF[tind + 1] = bufE[tind2 + 1];
                     bufF[tind + 2] = bufE[tind2 + 2];
                    
                     }
                
                 
                
            }
            if(uiMode == 4){
                
                    size_t colshift = icol * 3;
                    int in =0;
                    size_t tind =0;
                    size_t tind2 =0;
                    
                for( in = icol-1; in >=1; in--){
                
                 for( i = 0; i < iraw; i++){
                     tind = (in * 3 )  + (colshift * i);
                     tind2 = ((in - 1) * 3) + (colshift * i);
                     bufF[tind] = bufF[tind2];
                     bufF[tind + 1] = bufF[tind2 + 1];
                     bufF[tind + 2] = bufF[tind2 + 2];
                    }
                
                }// for ind
                    
                ind = numShift * 3;
                
                 for( i = 0; i < iraw; i++){
                    
                     tind = colshift * i;
                     tind2 = ind + (colshift * i);
                     bufF[tind ] = bufE[tind2];
                     bufF[tind + 1] = bufE[tind2 + 1];
                     bufF[tind + 2] = bufE[tind2 + 2];
                    
                     }
                
                 
                
            }
            showBuf();
            setTime();
        }
     
    stTicks--;
        
    }else{
        
        if( countCom > tekCom + 1 ){
            
            tekCom++;
           
    
         }else{
        
          tekCom = 0;
        }
        
   /* if( numCom == 3 || numCom == 4 ){
        
        for( ind = 0; ind < sizebuf; ind++){
            bufF[ind] = bufE[ind];
        }
        
        
    }*/
    
     StartCom();   
        
   }
    
}
///////////////////////////////////////////////////////
void setup(void) {
  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.print("\n");
  DBG_OUTPUT_PORT.setDebugOutput(true);
  SPIFFS.begin();
  strip.begin();  
  strip.setBrightness(255); 
  {

    listDir(SPIFFS, "/", 0);
    DBG_OUTPUT_PORT.printf("\n");
  }
  
//  listFiles(SPIFFS);
/* 
for( ind = 0; ind < icol; ind++){
      if(ind % 2 == 0){
          
          for( i = 0; i < iraw; i++){
              reind[ind*iraw + i] = (i * icol) + ind;
          }
          
      
          
      }else{
          
          for( i = 0; i < iraw; i++){
              reind[ind*iraw + i] = sizestrip - (i * icol) - icol + ind;
          }
          
      }
      
    
      
  }
  */  
  ///*
for( int ind = 0; ind < iraw; ind++){
 
      if(ind % 2 == 0){
          
          for( int i = icol - 1; i >= 0; i--){
              reind[icol * ind + i] = (ind * icol) + icol - 1 - i;
             
          }
          
      
          
      }else{
          
          for( i = 0; i < icol; i++){
              reind[icol * ind + i] = (ind * icol) + i;
             
          }
          
      }
      
    
      
  }
 
  //*/
  //WIFI INIT
  DBG_OUTPUT_PORT.printf("Connecting to %s\n", ssid);

  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  DBG_OUTPUT_PORT.print("AP IP address: ");
  DBG_OUTPUT_PORT.println(myIP);
  DBG_OUTPUT_PORT.println(ssid);


  //SERVER INIT
  //list directory
  server.on("/list", HTTP_GET, handleFileList) ;
  //load editor
  server.on("/edit", HTTP_GET, []() {
    if (!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");
  });
  //create file
  server.on("/edit", HTTP_PUT, handleFileCreate);
  //get params
  server.on("/", HTTP_PUT, handleGetParams);
  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, []() {
    server.send(200, "text/plain", "");
  }, handleFileUpload);

  //called when the url is not defined here
  //use it to load content from SPIFFS
  server.onNotFound([]() {
    if (!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
  });

  //get heap status, analog input value and all GPIO statuses in one json call
  server.on("/all", HTTP_GET, []() {
    String json = "{";
    json += "\"heap\":" + String(ESP.getFreeHeap());
    json += ", \"analog\":" + String(analogRead(A0));
    json += "}";
    server.send(200, "text/json", json);
    json = String();
  });
  server.begin();
  DBG_OUTPUT_PORT.println("HTTP server started");

}

void loop(void) {
    
  server.handleClient();
  
  if(timeThreshold > 0){
      if(lastTime == 0){
          lastTime = millis();
      }else{
          runTime = millis();
          if( runTime > lastTime){
              runTime = runTime - lastTime;
              if(runTime >= timeThreshold){
                  setTick();
              }
          }else{
              
              lastTime = millis();
          }
      }
  }
  if(changeregim){
      
      switch(iregim){
          case 0:
          setregFile();
          changeregim = false;
          break;
          case 1:
          setregTerminal();
          changeregim = false;
          break;
          default:
            switchOff();
            changeregim = false;
      }
  }
  if(changerange){
   size_t datt =0;
   unsigned char col0, col1, col2 = 0;
  
   for( ind = 0; ind < sizestrip; ind++){
       
       i = reind[ind]*3;
       datt = (irange * bufF[i])/100;
       col0 = (unsigned char) datt;
       datt = (irange * bufF[i + 1])/100;
       col1 = (unsigned char) datt;
       datt = (irange * bufF[i + 2])/100;
       col2 = (unsigned char) datt;
       
       strip.setPixelColor(ind, col0,  col1, col2);
       
       
   }
    
   strip.show(); 
   changerange = false;
  }
}
