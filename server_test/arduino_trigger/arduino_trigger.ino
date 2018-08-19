#define MAX_OUT_CHARS 255
char   buffer[MAX_OUT_CHARS + 1];
int output_pin = 13;
int frame_number = 1000;
int frame_id = 0;
bool flag = false;

void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(output_pin,OUTPUT);
}


void loop() 
{
  if(flag == true)
  {
   if(frame_id < frame_number)
    {
      delay(180);
      digitalWrite(output_pin,LOW);
      delay(20);
      digitalWrite(output_pin,HIGH);
      frame_id += 1;
      if(Serial.available())
      {
        String info;
        info = Serial.readStringUntil('\n');

        char str[50];
        info.toCharArray(str, 50);
        char * pch;
        pch = strtok (str,":");
        
        if(info.equals("e"))
        {
            flag = false;
            digitalWrite(output_pin,LOW);
        } 
      }
    }
    else
    {
      digitalWrite(output_pin,LOW);
      flag = false;
    }   
  }
  else
  {
    if(Serial.available())
    {
      String info;
      
      info = Serial.readStringUntil('\n');

      char str[50];
      info.toCharArray(str, 50);
      char * pch;
      pch = strtok (str,":");
      
      if(pch[0] == 's')
      {
          flag = true;
          pch = strtok(0, "&");
          frame_number = atoi(pch);

          Serial.print("frame_number is: ");
          Serial.println(frame_number);
      } 
    }
    
  }
  
  
}
