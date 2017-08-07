#!/bin/awk -f
BEGIN {

  Time = 0;
  Throughput = 0;
  Bitrate = 0;
  BufferSize = 0;
  ChunkCount = 0;
  VideoLevel = 0;
  ChunkCount = 0;
  DownloadTime = 0;

  state = 0;

  print "Time", "ChunkCount", "Bitrate", "DownloadTime" > "statistics1.dat";
  print "Time", "VideoLevel", "BufferSize" > "buffer1.dat";
  print "Time2", "ChunkCount2", "Bitrate2", "DownloadTime2" > "statistics2.dat";
  print "Time2", "VideoLevel2", "BufferSize2" > "buffer2.dat";
}

{
  if ($1 == "=======START===========") {
      state = 1;
  }
  else if ($1 == "=======BUFFER==========") {
      state = 2;
  }
  else if ($1 == "=======START2===========") {
      state = 3;
  }
  else if ($1 == "=======BUFFER2==========") {
      state = 4;
  }

  if ($1 == "Time:" && $3 == "ChunkCount:") {
    Time = $2/1000;
    Bitrate = $6;
    ChunkCount = $4;
    DownloadTime = $10;
    
    printf "%.2f %d %d %d\n", Time, ChunkCount, Bitrate, DownloadTime > "statistics1.dat";
  }

  if ($1 == "Time:" && $3 == "VideoLevel:") {
    Time = $2/1000;
    VideoLevel = $4;
    BufferSize = $6;

    printf "%.2f %d %.2f\n", Time, VideoLevel, BufferSize > "buffer1.dat";
  }

  if ($1 == "Time2:" && $3 == "ChunkCount2:") {
    Time = $2/1000;
    Bitrate = $6;
    ChunkCount = $4;
    DownloadTime = $10;
    
    printf "%.2f %d %d %d\n", Time, ChunkCount, Bitrate, DownloadTime > "statistics2.dat";
  }

  if ($1 == "Time2:" && $3 == "VideoLevel2:") {
    Time = $2/1000;
    VideoLevel = $4;
    BufferSize = $6;

    printf "%.2f %d %.2f\n", Time, VideoLevel, BufferSize > "buffer2.dat";
  }

}
END {
}



