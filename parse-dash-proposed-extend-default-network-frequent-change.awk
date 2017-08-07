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

  print "Time_extend", "target_extend", "bpsLastChunk_extend", "downloadDuration_extend", "requestInterval_extend", "Interval Ratio" > "statistics_extend_default_network_frequent_change.dat";
  print "Time_extend", "bufferSize_extend", "estimatedBW_extend", "videoLevel_extend" > "buffer_extend_default_network_frequent_change.dat";
  print "Time_extend2", "target_extend2", "bpsLastChunk_extend2", "downloadDuration_extend2", "requestInterval_extend2", "Interval Ratio2" > "statistics_extend_default_network_frequent_change2.dat";
  print "Time_extend2", "bufferSize_extend2", "estimatedBW_extend2", "videoLevel_extend2" > "buffer_extend_default_network_frequent_change2.dat";
}
{
  if ($1 == "=======START_extend===========") {
      state = 1;
  }
  else if ($1 == "=======BUFFER_extend==========") {
      state = 2;
  }
  else if ($1 == "=======START_extend2===========") {
      state = 3;
  } 
  else if ($1 == "=======BUFFER_extend2===========") {
      state = 4;
  }

  if ($1 == "Time_extend:" && $3 == "target_extend:") {
    Time = $2/1000;
    target = $4;
    bpsLastChunk = $6;
    downloadDuration = $8;
    requestInterval = $10;
    intervalRatio = $12;
    
    printf "%.2f %.2f %d %.2f %.2f %.2f\n", Time, target, bpsLastChunk, downloadDuration, requestInterval, intervalRatio > "statistics_extend_default_network_frequent_change.dat";
  }

  if ($1 == "Time_extend:" && $3 == "bufferSize_extend:") {
    Time = $2/1000;
    bufferSize = $4;
    estimatedBW = $6;
    videoLevel = $8;

    printf "%.2f %d %.2f %d\n", Time, bufferSize, estimatedBW, videoLevel > "buffer_extend_default_network_frequent_change.dat";
  }

  if ($1 == "Time_extend2:" && $3 == "target_extend2:") {
    Time = $2/1000;
    target = $4;
    bpsLastChunk = $6;
    downloadDuration = $8;
    requestInterval = $10;
    intervalRatio = $12;
    
    printf "%.2f %.2f %d %.2f %.2f %.2f\n", Time, target, bpsLastChunk, downloadDuration, requestInterval, intervalRatio > "statistics_extend_default_network_frequent_change2.dat";
  }

  if ($1 == "Time_extend2:" && $3 == "bufferSize_extend2:") {
    Time = $2/1000;
    bufferSize = $4;
    estimatedBW = $6;
    videoLevel = $8;

    printf "%.2f %d %.2f %d\n", Time, bufferSize, estimatedBW, videoLevel > "buffer_extend_default_network_frequent_change2.dat";
  }
}
END {
}



