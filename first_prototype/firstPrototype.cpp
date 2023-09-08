int duty_cycle = 255; // 0 - 255;

unsigned long avg_16_ms() {
  unsigned long sum =0, count = 0, avg, t_start = millis();
  for(count=0; millis() - t_start < 16; count++) {  // 16ms because AC is 60Hz frequency ; attempt to eliminate ext. light interference
    sum += analogRead(A0);
  }
  avg = sum / count;
  return avg;
}

float calc_R(unsigned long ir_min, unsigned long ir_max, unsigned long r_min, unsigned long r_max) {
  return ((r_max - r_min) / (float)r_min) / ((ir_max - ir_min) / (float)ir_min);
}

void setup() {
  Serial.begin(9600);
  Serial.flush();
  pinMode(3, OUTPUT); // IR LED
  pinMode(7, OUTPUT); // heartbeat LED
  pinMode(5, OUTPUT); // red LED
  analogWrite(3, 0);
  digitalWrite(7, LOW); 
  digitalWrite(5, LOW);
  
}

#define m_length 10
#define rise_threshold 2
#define o_length 20
#define R_length 5

int count, rise_count = 0, measure_count = 0, bpm, o_count = 0, SpO2;
unsigned long ir_avg = 0, r_avg = 0, last_avg, last_beat = 0, measure_sum, r_min = 0, r_max = 0, ir_min = 0, ir_max = 0;
bool rising = false;
unsigned long measures[m_length] = {0}, red_measures[o_length] = {0}, ir_measures[o_length] = {0};
float R, R_sum = 0, R_avg, R_count;



void loop() {
   
  last_avg = ir_avg;

  // take average of ir led
  analogWrite(3, duty_cycle); // Apply duty cycle to IR light      16
  delay(3); // allow light to turn on fully

  ir_avg = avg_16_ms();
  
  while(ir_avg < 25) {
    Serial.println("waiting for finger...");
    ir_avg = avg_16_ms();
  }

  analogWrite(3, 0);


  digitalWrite(7, LOW); // heartbeat led, turn off

  // Now take average using red led
  digitalWrite(5, HIGH);
  delay(3); // allow light to turn on fully

  r_avg = avg_16_ms();

  digitalWrite(5, LOW);

  if(r_avg > ir_avg) { // if finger in sensor
    // record average from both IR & Red light for Sp02.
    red_measures[o_count] = r_avg;
    ir_measures[o_count++] = ir_avg;
  
    // determine max & min
    if(r_avg > r_max){
      r_max = r_avg;
    } else if (ir_avg > ir_max) {
      ir_max = ir_avg;
    }
    if(r_avg < r_min || r_min == 0) {
      r_min = r_avg;
    } else if(ir_avg < ir_min || ir_min == 0) {
      ir_min = ir_avg;
    }
    
    if(o_count + 1 == o_length) {
      R = calc_R(ir_min, ir_max, r_min, r_max);
      ir_min = ir_max = r_min = r_max = 0; // reset max & min values
      o_count = 0;
      // add calculated R value to sum
      R_sum += R;
      R_count++;
      if(R_count == R_length) { // calculate avg if enough samples have been taken
          R_avg = R_sum / R_count;
          R_count = R_sum = 0;
          SpO2 = (-15 * R) + 100;     // Spo2 = m * R + b
          Serial.print("SpO2: ");
          Serial.println(SpO2);
      }
    }
  }



  if (ir_avg > last_avg) { // if pulse is rising
    rise_count++;
    rising = true;
  } else {
    if (rising && rise_count >= rise_threshold && last_beat != 0) { // if has been rising for at least rise_threshold times and is now falling
      measures[measure_count++] = millis() - last_beat;
      last_beat = millis();
      if(measures[measure_count-1] < 300 || measures[measure_count-1] > 2222) { // Exclude >200bpm and <27bpm
         measure_count--;
       } else {
         digitalWrite(7, HIGH); // heartbeat led ; only turn on if measure was valid
       }
      if(measure_count == m_length) { // if we have m_length number of samples, calculate average
        measure_sum = 0;
        
        for(int i=0; i<m_length; i++)
        {
          // skip if number is larger than 20% of next value
          if(i!=m_length-1 && measures[i] > measures[i+1] * 1.2) {
            measure_count--;
            continue;
          }
          measure_sum += measures[i];
        }
        measure_sum /= measure_count;
        bpm = 60000 / measure_sum;
        if(measure_count >= 7) { // if there is at least 7 good measures
          Serial.print("BPM: ");
          Serial.println(bpm);
        }      
        measure_count = 0; 
      }
    } else if(rising && rise_count >= rise_threshold && last_beat == 0) { // if this is first beat recorded
      last_beat = millis();
    }
    rise_count = 0;
    rising = false;
  }
}