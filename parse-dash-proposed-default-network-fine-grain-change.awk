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

  print "Time_proposed", "target_proposed", "bpsAverage_proposed", "bpsLastChunk_proposed", "lastBitrate_proposed", "chunkCount_proposed", "totalChunks_proposed", "downloadDuration_proposed", "requestInterval_proposed" > "statistics_proposed_default_network_fine_grain_change.dat";
  print "Time_proposed", "bufferSize_proposed", "hasThroughput_proposed", "estimatedBW_proposed", "videoLevel_proposed", "tcpThroughput_proposed" > "buffer_proposed_default_network_fine_grain_change.dat";
  print "Time_proposed2", "target_proposed2", "bpsAverage_proposed2", "bpsLastChunk_proposed2", "lastBitrate_proposed2", "chunkCount_proposed2", "totalChunks_proposed2", "downloadDuration_proposed2", "requestInterval_proposed2" > "statistics_proposed_default_network_fine_grain_change2.dat";
  print "Time_proposed2", "bufferSize_proposed2", "hasThroughput_proposed2", "estimatedBW_proposed2", "videoLevel_proposed2", "tcpThroughput_proposed2" > "buffer_proposed_default_network_fine_grain_change2.dat";
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

  if ($1 == "Time_proposed:" && $3 == "bpsAverage_proposed:") {
    Time = $2/1000;
    target = $4;
    bpsAverage = $6;
    bpsLastChunk = $8;
    lastBitrate = $10;
    chunkCount = $12;
    totalChunks = $14;
    downloadDuration = $16;
    requestInterval = $18;
    
    printf "%.2f %.2f %.2f %d %d %d %d %.2f %.2f\n", Time, target, bpsAverage, bpsLastChunk, lastBitrate, chunkCount, totoalChunks, downloadDuration, requestInterval > "statistics_proposed_default_network_fine_grain_change.dat";
  }

  if ($1 == "Time_proposed:" && $3 == "bufferSize_proposed:") {
    Time = $2/1000;
    bufferSize = $4;
    hasThroughput = $6;
    estimatedBW = $8;
    videoLevel = $10;
    tcpThroughput = $12;

    printf "%.2f %d %.2f %.2f %d %.2f\n", Time, bufferSize, hasThroughput, estimatedBW, videoLevel, tcpThroughput > "buffer_proposed_default_network_fine_grain_change.dat";
  }

  if ($1 == "Time_proposed2:" && $3 == "bpsAverage_proposed2:") {
    Time = $2/1000;
    target = $4;
    bpsAverage = $6;
    bpsLastChunk = $8;
    lastBitrate = $10;
    chunkCount = $12;
    totalChunks = $14;
    downloadDuration = $16;
    requestInterval = $18;
    
    printf "%.2f %.2f %.2f %d %d %d %d %.2f %.2f\n", Time, target, bpsAverage, bpsLastChunk, lastBitrate, chunkCount, totoalChunks, downloadDuration, requestInterval > "statistics_proposed_default_network_fine_grain_change2.dat";
  }

  if ($1 == "Time_proposed2:" && $3 == "bufferSize_proposed2:") {
    Time = $2/1000;
    bufferSize = $4;
    hasThroughput = $6;
    estimatedBW = $8;
    videoLevel = $10;
    tcpThroughput = $12;

    printf "%.2f %d %.2f %.2f %d %.2f\n", Time, bufferSize, hasThroughput, estimatedBW, videoLevel, tcpThroughput > "buffer_proposed_default_network_fine_grain_change2.dat";
  }

}
END {
}



