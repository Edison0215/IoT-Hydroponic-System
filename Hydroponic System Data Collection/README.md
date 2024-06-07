# Hydroponic System Data Collection
- Data are collected via ThingSpeak.
- Enviromental database: https://thingspeak.com/channels/2320335
- Nutrient ECPH database: https://thingspeak.com/channels/2373435

## Environmental Data Collection Excel File
- Initially, data are collected via ThingSpeak at a rate of 15 seconds.
- Data of each parameter are further processed to determine an average reading of 1 hour.
- To manually install data from ThingSpeak at a rate of 1 hour from the ThingSpeak channel
- For example, collecting hourly data of each environmental parameter on 14 February 2024, paste the link below in your web browser:
```https://api.thingspeak.com/channels/2320335/feeds.csv?start=2024-02-14&end=2024-02-15&average=60&timezone=Asia%2FKuala_Lumpur```
- If you want to change to another day of data, just change the ```start data``` and ```end data``` of the link

## Nutrient ECPH Data Collection Excel File
- ECPH Control = 1: Nutrient regulation is activated when ECPHDOWN is prompted into serial monitor.
- ECPH Control = 0: Nutrient regulaton is deactivated when ECPHUP is prompted into serial monitor.

## Nutrient Regulation Process Excel File
- Since the nutrient condition is always optimal during the 60 days cultivation, the reservoir is purposely diluted with water to determine the system nutrient regulation capability on 20/3/2024.
- Optimal EC range set in source code: 0.8 mS/cm ~ 2.0 mS/cm
- Optimal pH range set in source code: pH 6 ~ pH 8
