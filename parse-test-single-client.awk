#!/bin/awk -f
BEGIN {

  Time = 0;
  BufferSize = 0;
  ChunkCount = 0;
  VideoLevel = 0;
  DownloadTime = 0;
  RequestInterval = 0;
  BufferVariation = 0;

  state = 0;

  print "Time", "BufferSize", "ChunkCount", "DownloadTime", "RequestInterval", "BufferVariation" > "statistics_single.dat";
  print "Time", "BufferSize", "VideoLevel", "DownloadTime", "RequestInterval", "BufferVariation" > "buffer_single.dat";
}

{
  if ($1 == "=====START=====") {
      state = 1;
  }
  else if ($1 == "=====BUFFER=====") {
      state = 2;
  }

  if ($1 == "Time:" && $5 == "ChunkCount:") {
    Time = $2/1000;
    BufferSize = $4;
    ChunkCount = $6;
    DownloadTime = $8;
    RequestInterval = $10;
    BufferVariation = $12;
    
    printf "%.2f %.2f %d %.2f %.2f %.2f\n", Time, BufferSize, ChunkCount, DownloadTime, RequestInterval, BufferVariation > "statistics_single.dat";
  }

  if ($1 == "Time:" && $5 == "VideoLevel:") {
    Time = $2/1000;
    BufferSize = $4;
    VideoLevel = $6;
    DownloadTime = $8;
    RequestInterval = $10;
    BufferVariation = $12;

    printf "%.2f %.2f %d %.2f %.2f %.2f\n", Time, BufferSize, VideoLevel, DownloadTime, RequestInterval, BufferVariation > "buffer_single.dat";
  }

}
END {
}



