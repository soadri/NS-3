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

  print "Time", "target", "bpsAverage", "bpsLastChunk", "lastBitrate", "chunkCount", "totalChunks", "downloadDuration", "requestInterval" > "statistics_panda.dat";
  print "Time", "bufferSize", "hasThroughput", "estimatedBW", "videoLevel", "tcpThroughput" > "buffer_panda.dat";
}
{
  if ($1 == "=======START===========") {
      state = 1;
  }
  else if ($1 == "=======BUFFER==========") {
      state = 2;
  }

  if ($1 == "Time:" && $3 == "bpsAverage:") {
    Time = $2/1000;
    target = $4;
    bpsAverage = $6;
    bpsLastChunk = $8;
    lastBitrate = $10;
    chunkCount = $12;
    totalChunks = $14;
    downloadDuration = $16;
    requestInterval = $18;
    
    printf "%.2f %.2f %.2f %d %d %d %d %.2f %.2f\n", Time, target, bpsAverage, bpsLastChunk, lastBitrate, chunkCount, totoalChunks, downloadDuration, requestInterval > "statistics_panda.dat";
  }

  if ($1 == "Time:" && $3 == "bufferSize:") {
    Time = $2/1000;
    bufferSize = $4;
    hasThroughput = $6;
    estimatedBW = $8;
    videoLevel = $10;
    tcpThroughput = $12;

    printf "%.2f %d %.2f %.2f %d %.2f\n", Time, bufferSize, hasThroughput, estimatedBW, videoLevel, tcpThroughput > "buffer_panda.dat";
  }
}
END {
}



