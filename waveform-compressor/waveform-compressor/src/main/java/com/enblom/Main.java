package com.enblom;

import java.io.IOException;
import java.nio.file.Path;

public class Main {

  public static void main(String[] args) {
    Compressor compressor = new Compressor();
    try {
      var compressed =
          compressor.compress(
              Path.of(
                  "/Users/henblom/projects/github/henrikenblom/SIDPod/src/audio/reSID/wave6581_PST.cc"));
      for (int i = 0; i < 64; i++) {
        if (i % 16 == 0) {
          System.out.println();
        }
        System.out.print(String.format("0x%02x, ", compressed.values.get(i)));
      }
      System.out.println();
      System.out.println();
      System.out.println(compressed);
      for (int i = 0; i < 4096; i++) {
        if (i % 8 == 0) {
          System.out.print(String.format("\n/* 0x%03x: */  0x%02x, ", i, compressed.getValue(i)));
        } else {
          System.out.print(String.format("0x%02x, ", compressed.getValue(i)));
        }
      }
    } catch (IOException e) {
      e.printStackTrace();
    }
  }
}
