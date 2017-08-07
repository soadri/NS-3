#!/bin/awk -f
BEGIN {

  Time = 0;
  target = 0;
  bpsLastChunk = 0;
  downloadDuration = 0;
  requestInterval = 0;
  intervalRatio = 0;

  bufferSize = 0;
  estimatedBW = 0;
  videoLevel = 0;

  state = 0;

  print "Time_proposed", "target_proposed", "bpsLastChunk_proposed", "downloadDuration_proposed", "requestInterval_proposed", "Interval Ratio" > "statistics_proposed_default_no_network_change.dat";
  print "Time_proposed", "bufferSize_proposed", "estimatedBW_proposed", "videoLevel_proposed" > "buffer_proposed_default_no_network_change.dat";
  print "Time_proposed2", "target_proposed2", "bpsLastChunk_proposed2", "downloadDuration_proposed2", "requestInterval_proposed2", "Interval Ratio2" > "statistics_proposed_default_no_network_change2.dat";
  print "Time_proposed2", "bufferSize_proposed2", "estimatedBW_proposed2", "videoLevel_proposed2" > "buffer_proposed_default_no_network_change2.dat";
}
{
  if ($1 == "=======START_PROPOSED===========") {
      state = 1;
  }
  else if ($1 == "=======BUFFER_PROPOSED==========") {
      state = 2;
  }
  else if ($1 == "=======START_PROPOSED2===========") {
      state = 3;
  } 
  else if ($1 == "=======BUFFER_PROPOSED2===========") {
      state = 4;
  }

  if ($1 == "Time_proposed:" && $3 == "target_proposed:") {
    Time = $2/1000;
    target = $4;
    bpsLastChunk = $6;
    downloadDuration = $8;
    requestInterval = $10;
    intervalRatio = $12;
    
    printf "%.2f %.2f %d %.2f %.2f %.2f\n", Time, target, bpsLastChunk, downloadDuration, requestInterval, intervalRatio > "statistics_proposed_default_no_network_change.dat";
  }

  if ($1 == "Time_proposed:" && $3 == "bufferSize_proposed:") {
    Time = $2/1000;
    bufferSize = $4;
    estimatedBW = $6;
    videoLevel = $8;

    printf "%.2f %d %.2f %d\n", Time, bufferSize, estimatedBW, videoLevel > "buffer_proposed_default_no_network_change.dat";
  }

  if ($1 == "Time_proposed2:" && $3 == "target_proposed2:") {
    Time = $2/1000;
    target = $4;
    bpsLastChunk = $6;
    downloadDuration = $8;
    requestInterval = $10;
    intervalRatio = $12;
    
    printf "%.2f %.2f %d %.2f %.2f %.2f\n", Time, target, bpsLastChunk, downloadDuration, requestInterval, intervalRatio > "statistics_proposed_default_no_network_change2.dat";
  }

  if ($1 == "Time_proposed2:" && $3 == "bufferSize_proposed2:") {
    Time = $2/1000;
    bufferSize = $4;
    estimatedBW = $6;
    videoLevel = $8;

    printf "%.2f %d %.2f %d\n", Time, bufferSize, estimatedBW, videoLevel > "buffer_proposed_default_no_network_change2.dat";
  }
}
END {
}



