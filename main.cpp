#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "lallinger";
const char* password = "Fritz9.3.1990";
const char* mqtt_server = "192.168.0.210";

const char* clientId = "FFT01";

#define uS_TO_S_FACTOR 1000000  //haben wir aus dem Beispiel
#define TIME_TO_SLEEP  2100 //nomen est....

float FFT(int in[],int N,float Frequency);
float cosine(int i);
float sine(int i);
void callback(char* topic, byte* payload, unsigned int length);
void setup_wifi_local(void);
void reconnect(void);

//Example data:
int data[256]={};

char timestring[50];
char msg[50];
char fvalue[100];
long lastMsg = 0;
unsigned int cnt = 0;
char buf [5];

WiFiClient espClient;
PubSubClient client(espClient);

//---------------------------------------------------------------------------//
byte sine_data [91]=
 {
0,  
4,    9,    13,   18,   22,   27,   31,   35,   40,   44, 
49,   53,   57,   62,   66,   70,   75,   79,   83,   87, 
91,   96,   100,  104,  108,  112,  116,  120,  124,  127,  
131,  135,  139,  143,  146,  150,  153,  157,  160,  164,  
167,  171,  174,  177,  180,  183,  186,  189,  192,  195,       //Paste this at top of program
198,  201,  204,  206,  209,  211,  214,  216,  219,  221,  
223,  225,  227,  229,  231,  233,  235,  236,  238,  240,  
241,  243,  244,  245,  246,  247,  248,  249,  250,  251,  
252,  253,  253,  254,  254,  254,  255,  255,  255,  255
  };
float f_peaks[5]; // top 5 frequencies peaks in descending order
//---------------------------------------------------------------------------//


void setup() 
        {
        Serial.begin(9600);
        delay(500);

        setup_wifi_local();
        client.setServer(mqtt_server, 1883);
        client.setCallback(callback);

        for (int i=0;i<256;i++){
          data[i]=analogRead(34);
          delay(1);
        }

        if (!client.connected()) {
          reconnect();
        }
        client.loop();
        strcpy(msg,clientId);
        strcat(msg,"/outTopic/IP");
        client.publish(msg, WiFi.localIP().toString().c_str());

        for (int i=0;i<10;i++){
          for (int j=0;j<256;j++){
            data[j]=analogRead(34);
            delay(1);
          }
          FFT(data,256,400);        //to get top five value of frequencies of X having 64 sample at 100Hz sampling
/*        Serial.println(f_peaks[0]);
        Serial.println(f_peaks[1]);
        Serial.println(f_peaks[2]);
        Serial.println(f_peaks[3]);
        Serial.println(f_peaks[4]);
        Serial.println("=========================================");  */

          strcpy(msg,clientId);
          strcat(msg,"/outTopic/FFTvalues");
          sprintf(fvalue,"%f;%f,%f;%f;%f",f_peaks[0],f_peaks[1],f_peaks[2],f_peaks[3],f_peaks[4]);
          client.publish(msg, fvalue);
          delay(1000);
        }

        esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR); //stellt den Timer
        esp_deep_sleep_start(); //Gute Nacht!
        }

        
void loop() {

            }



//-----------------------------FFT Function----------------------------------------------//

float FFT(int in[],int N,float Frequency)
{
/*
Code to perform FFT on arduino,
setup:
paste sine_data [91] at top of program [global variable], paste FFT function at end of program
Term:
1. in[]     : Data array, 
2. N        : Number of sample (recommended sample size 2,4,8,16,32,64,128...)
3. Frequency: sampling frequency required as input (Hz)

If sample size is not in power of 2 it will be clipped to lower side of number. 
i.e, for 150 number of samples, code will consider first 128 sample, remaining sample  will be omitted.
For Arduino nano, FFT of more than 128 sample not possible due to mamory limitation (64 recomended)
For higher Number of sample may arise Mamory related issue,
Code by ABHILASH
Contact: abhilashpatel121@gmail.com 
Documentation:https://www.instructables.com/member/abhilash_patel/instructables/
2/3/2021: change data type of N from float to int for >=256 samples
*/

unsigned int data[13]={1,2,4,8,16,32,64,128,256,512,1024,2048};
int a,c1,f,o,x;
a=N;  
                                 
      for(int i=0;i<12;i++)                 //calculating the levels
         { if(data[i]<=a){o=i;} }
      
int in_ps[data[o]]={};     //input for sequencing
float out_r[data[o]]={};   //real part of transform
float out_im[data[o]]={};  //imaginory part of transform
           
x=0;  
      for(int b=0;b<o;b++)                     // bit reversal
         {
          c1=data[b];
          f=data[o]/(c1+c1);
                for(int j=0;j<c1;j++)
                    { 
                     x=x+1;
                     in_ps[x]=in_ps[j]+f;
                    }
         }

 
      for(int i=0;i<data[o];i++)            // update input array as per bit reverse order
         {
          if(in_ps[i]<a)
          {out_r[i]=in[in_ps[i]];}
          if(in_ps[i]>a)
          {out_r[i]=in[in_ps[i]-a];}      
         }


int i10,i11,n1;
float e,c,s,tr,ti;

    for(int i=0;i<o;i++)                                    //fft
    {
     i10=data[i];              // overall values of sine/cosine  :
     i11=data[o]/data[i+1];    // loop with similar sine cosine:
     e=360/data[i+1];
     e=0-e;
     n1=0;

          for(int j=0;j<i10;j++)
          {
          c=cosine(e*j);
          s=sine(e*j);    
          n1=j;
          
                for(int k=0;k<i11;k++)
                 {
                 tr=c*out_r[i10+n1]-s*out_im[i10+n1];
                 ti=s*out_r[i10+n1]+c*out_im[i10+n1];
          
                 out_r[n1+i10]=out_r[n1]-tr;
                 out_r[n1]=out_r[n1]+tr;
          
                 out_im[n1+i10]=out_im[n1]-ti;
                 out_im[n1]=out_im[n1]+ti;          
          
                 n1=n1+i10+i10;
                  }       
             }
     }

/*
for(int i=0;i<data[o];i++)
{
Serial.print(out_r[i]);
Serial.print("\t");                                     // un comment to print RAW o/p    
Serial.print(out_im[i]); Serial.println("i");      
}
*/


//---> here onward out_r contains amplitude and our_in conntains frequency (Hz)
    for(int i=0;i<data[o-1];i++)               // getting amplitude from compex number
        {
         out_r[i]=sqrt(out_r[i]*out_r[i]+out_im[i]*out_im[i]); // to  increase the speed delete sqrt
         out_im[i]=i*Frequency/N;
         /*
         Serial.print(out_im[i]); Serial.print("Hz");
         Serial.print("\t");                            // un comment to print freuency bin    
         Serial.println(out_r[i]); 
         */    
        }




x=0;       // peak detection
   for(int i=1;i<data[o-1]-1;i++)
      {
      if(out_r[i]>out_r[i-1] && out_r[i]>out_r[i+1]) 
      {in_ps[x]=i;    //in_ps array used for storage of peak number
      x=x+1;}    
      }


s=0;
c=0;
    for(int i=0;i<x;i++)             // re arraange as per magnitude
    {
        for(int j=c;j<x;j++)
        {
            if(out_r[in_ps[i]]<out_r[in_ps[j]]) 
                {s=in_ps[i];
                in_ps[i]=in_ps[j];
                in_ps[j]=s;}
        }
    c=c+1;
    }



    for(int i=0;i<5;i++)     // updating f_peak array (global variable)with descending order
    {
    f_peaks[i]=out_im[in_ps[i]];
    }



}
    

float sine(int i)
{
  int j=i;
  float out;
  while(j<0){j=j+360;}
  while(j>360){j=j-360;}
  if(j>-1   && j<91){out= sine_data[j];}
  else if(j>90  && j<181){out= sine_data[180-j];}
  else if(j>180 && j<271){out= -sine_data[j-180];}
  else if(j>270 && j<361){out= -sine_data[360-j];}
  return (out/255);
}

float cosine(int i)
{
  int j=i;
  float out;
  while(j<0){j=j+360;}
  while(j>360){j=j-360;}
  if(j>-1   && j<91){out= sine_data[90-j];}
  else if(j>90  && j<181){out= -sine_data[j-90];}
  else if(j>180 && j<271){out= -sine_data[270-j];}
  else if(j>270 && j<361){out= sine_data[j-270];}
  return (out/255);
}

//------------------------------------------------------------------------------------//

void setup_wifi_local() {
  int loopcnt = 0;
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    loopcnt++;
    if (loopcnt > 20){
      ESP.restart();
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(clientId)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
