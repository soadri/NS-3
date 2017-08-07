#!/bin/awk -f
BEGIN {

  Time = 0;
  target = 0;
  bpsAverage = 0;
  bpsLastChunk = 0;
  lastBitrate = 0;
  chunkCount = 0;
  totalChunks = 0;
  downloadDuration = 0;
  requestInterval = 0;

  bufferSize = 0;
  hasThroughput = 0;
  estimatedBW = 0;
  videoLevel = 0;
  tcpThroughput = 0;

  state = 0;

  print "Time_panda", "target_panda", "bpsAverage_panda", "bpsLastChunk_panda", "lastBitrate_panda", "chunkCount_panda", "totalChunks_panda", "downloadDuration_panda", "requestInterval_panda" > "statistics_panda_default_network_fine_grain_change.dat";
  print "Time_panda", "bufferSize_panda", "hasThroughput_panda", "estimatedBW_panda", "videoLevel_panda", "tcpThroughput_panda" > "buffer_panda_default_network_fine_grain_change.dat";
  print "Time_panda2", "target_panda2", "bpsAverage_panda2", "bpsLastChunk_panda2", "lastBitrate_panda2", "chunkCount_panda2", "totalChunks_panda2", "downloadDuration_panda2", "requestInterval_panda2" > "statistics_panda_default_network_fine_grain_change2.dat";
  print "Time_panda2", "bufferSize_panda2", "hasThroughput_panda2", "estimatedBW_panda2", "videoLevel_panda2", "tcpThroughput_panda2" > "buffer_panda_default_network_fine_grain_change2.dat";
}
{
  if ($1 == "=======START_PANDA===========") {
      state = 1;
  }
  else if ($1 == "=======BUFFER_PANDA==========") {
      state = 2;
  }
  else if ($1 == "=======START_PANDA2===========") {
      state = 3;
  } 
  else if ($1 == "=======BUFFER_PANDA2===========") {
      state = 4;
  }

  if ($1 == "Time_panda:" && $3 == "bpsAverage_panda:") {
    Time = $2/1000;
    target = $4;
    bpsAverage = $6;
    bpsLastChunk = $8;
    lastBitrate = $10;
    chunkCount = $12;
    totalChunks = $14;
    downloadDuration = $16;
    requestInterval = $18;
    
    printf "%.2f %.2f %.2f %d %d %d %d %.2f %.2f\n", Time, target, bpsAverage, bpsLastChunk, lastBitrate, chunkCount, totoalChunks, downloadDuration, requestInterval > "statistics_panda_default_network_fine_grain_change.dat";
  }

  if ($1 == "Time_panda:" && $3 == "bufferSize_panda:") {
    Time = $2/1000;
    bufferSize = $4;
    hasThroughput = $6;
    estimatedBW = $8;
    videoLevel = $10;
    tcpThroughput = $12;

    printf "%.2f %d %.2f %.2f %d %.2f\n", Time, bufferSize, hasThroughput, estimatedBW, videoLevel, tcpThroughput > "buffer_panda_default_network_fine_grain_change.dat";
  }

  if ($1 == "Time_panda2:" && $3 == "bpsAverage_panda2:") {
    Time = $2/1000;
    target = $4;
    bpsAverage = $6;
    bpsLastChunk = $8;
    lastBitrate = $10;
    chunkCount = $12;
    totalChunks = $14;
    downloadDuration = $16;
    requestInterval = $18;
    
    printf "%.2f %.2f %.2f %d %d %d %d %.2f %.2f\n", Time, target, bpsAverage, bpsLastChunk, lastBitrate, chunkCount, totoalChunks, downloadDuration, requestInterval > "statistics_panda_default_network_fine_grain_change2.dat";
  }

  if ($1 == "Time_panda2:" && $3 == "bufferSize_panda2:") {
    Time = $2/1000;
    bufferSize = $4;
    hasThroughput = $6;
    estimatedBW = $8;
    videoLevel = $10;
    tcpThroughput = $12;

    printf "%.2f %d %.2f %.2f %d %.2f\n", Time, bufferSize, hasThroughput, estimatedBW, videoLevel, tcpThroughput > "buffer_panda_default_network_fine_grain_change2.dat";
  }

}
END {
}



