package com.enblom;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.stream.Collectors;
import java.util.stream.Stream;

public class SwitchStatementGenerator {
  private Stream<String> getData(Path path) throws IOException {
    return Files.lines(path);
  }

  private Stream<String> filter(Stream<String> data) {
    return data.filter(line -> line.startsWith("/* 0x")).map(line -> line.substring(14));
  }

  private Stream<String> format(Stream<String> data) {
    AtomicBoolean newLine = new AtomicBoolean(false);
    return data.map(line -> line.replace(",", "")).flatMap(line -> Arrays.stream(line.split(" ")));
  }

  private String extractName(Path path) {
    String fileName = path.getFileName().toString();
    return fileName.substring(0, fileName.lastIndexOf('.'));
  }

  public String generate(String pathString) throws IOException {
    var path = Path.of(pathString);
    HashMap<String, List<Integer>> map = new HashMap<>();
    AtomicInteger input = new AtomicInteger(0);
    format(filter(getData(path)))
        .forEach(
            value -> {
              if (!value.equals("0x00")) {
                if (!map.containsKey(value)) {
                  var list = new ArrayList<Integer>();
                  list.add(input.get());
                  map.put(value, list);
                } else {
                  map.get(value).add(input.get());
                }
              }
              input.getAndIncrement();
            });
    StringBuilder sb = new StringBuilder();
    sb.append("RESID_INLINE\n");
    sb.append("reg12 ").append(extractName(path)).append("resolve(reg12 output) {\n");
    sb.append("\tswitch (output) {\n");
    map.forEach(
        (key, value) -> {
          sb.append(
              value.stream()
                  .map(Integer::toHexString)
                  .map(hex -> "\t\tcase 0x" + hex)
                  .collect(Collectors.joining(":\n")));
          sb.append(":\n").append("\t\t\treturn ").append(key);
          sb.append(";\n");
        });
    sb.append("\t\tdefault:\n\t\t\treturn 0;\n\t}\n}\n");
    return sb.toString();
  }

  public static void main(String[] args) {
    SwitchStatementGenerator generator = new SwitchStatementGenerator();
    try {
    System.out.println(generator.generate("/Users/henblom/projects/github/henrikenblom/SIDPod/src/audio/reSID/wave6581_PST.cc"));
    System.out.println(generator.generate("/Users/henblom/projects/github/henrikenblom/SIDPod/src/audio/reSID/wave6581_PS_.cc"));
    System.out.println(generator.generate("/Users/henblom/projects/github/henrikenblom/SIDPod/src/audio/reSID/wave6581_P_T.cc"));
    System.out.println(generator.generate("/Users/henblom/projects/github/henrikenblom/SIDPod/src/audio/reSID/wave6581__ST.cc"));
    System.out.println(generator.generate("/Users/henblom/projects/github/henrikenblom/SIDPod/src/audio/reSID/wave8580_PST.cc"));
    System.out.println(generator.generate("/Users/henblom/projects/github/henrikenblom/SIDPod/src/audio/reSID/wave8580_PS_.cc"));
    System.out.println(generator.generate("/Users/henblom/projects/github/henrikenblom/SIDPod/src/audio/reSID/wave8580_P_T.cc"));
    System.out.println(generator.generate("/Users/henblom/projects/github/henrikenblom/SIDPod/src/audio/reSID/wave8580__ST.cc"));
    } catch (IOException e) {
      throw new RuntimeException(e);
    }
  }
}
