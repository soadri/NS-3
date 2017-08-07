#!/bin/awk -f
BEGIN {

  Time = 0;
  BufferSize = 0;
  ChunkCount = 0;
  VideoLevel = 0;
  DownloadTime = 0;
  RequestInterval = 0;
  BufferVariation = 0;
  PreviousQuality = 0;

  state = 0;

  print "Time", "BufferSize", "ChunkCount", "DownloadTime", "RequestInterval", "BufferVariation" > "statistics_multiple1.dat";
  print "Time", "BufferSize", "VideoLevel", "DownloadTime", "RequestInterval", "BufferVariation, PreviousQuality" > "buffer_multiple1.dat";
  print "Time2", "BufferSize2", "ChunkCount2", "DownloadTime2", "RequestInterval2", "BufferVariation2" > "statistics_multiple2.dat";
  print "Time2", "BufferSize2", "VideoLevel2", "DownloadTime2", "RequestInterval2", "BufferVariation2, PreviousQuality2" > "buffer_multiple2.dat";
}

{
  if ($1 == "=====START=====") {
      state = 1;
  }
  else if ($1 == "=====BUFFER=====") {
      state = 2;
  }
  else if ($1 == "=====START2=====") {
      state = 3;
  }
  else if ($1 == "=====BUFFER2=====") {
      state = 4;
  }

  if ($1 == "Time:" && $5 == "ChunkCount:") {
    Time = $2/1000;
    BufferSize = $4;
    ChunkCount = $6;
    DownloadTime = $8;
    RequestInterval = $10;
    BufferVariation = $12;
    
    printf "%.2f %.2f %d %.2f %.2f %.2f\n", Time, BufferSize, ChunkCount, DownloadTime, RequestInterval, BufferVariation > "statistics_multiple1.dat";
  }

  if ($1 == "Time:" && $5 == "VideoLevel:") {
    Time = $2/1000;
    BufferSize = $4;
    VideoLevel = $6;
    DownloadTime = $8;
    RequestInterval = $10;
    BufferVariation = $12;
    PreviousQuality = $14;

    printf "%.2f %.2f %d %.2f %.2f %.2f %d\n", Time, BufferSize, VideoLevel, DownloadTime, RequestInterval, BufferVariation, PreviousQuality > "buffer_multiple1.dat";
  }

 if ($1 == "Time2:" && $5 == "ChunkCount2:") {
    Time = $2/1000;
    BufferSize = $4;
    ChunkCount = $6;
    DownloadTime = $8;
    RequestInterval = $10;
    BufferVariation = $12;
    
    printf "%.2f %.2f %d %.2f %.2f %.2f\n", Time, BufferSize, ChunkCount, DownloadTime, RequestInterval, BufferVariation > "statistics_multiple2.dat";
  }

  if ($1 == "Time2:" && $5 == "VideoLevel2:") {
    Time = $2/1000;
    BufferSize = $4;
    VideoLevel = $6;
    DownloadTime = $8;
    RequestInterval = $10;
    BufferVariation = $12;
    PreviousQuality = $14;

    printf "%.2f %.2f %d %.2f %.2f %.2f %d\n", Time, BufferSize, VideoLevel, DownloadTime, RequestInterval, BufferVariation, PreviousQuality > "buffer_multiple2.dat";
  }


}
END {
}



