package com.enblom;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Map.Entry;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.stream.Stream;

public class Compressor {

  private Stream<String> getData(Path path) throws IOException {
    return Files.lines(path);
  }

  private Stream<String> filter(Stream<String> data) {
    return data.filter(line -> line.startsWith("/* 0x")).map(line -> line.substring(14));
  }

  private Stream<String> format(Stream<String> data) {
    AtomicBoolean newLine = new AtomicBoolean(false);
    return Stream.of(
        data.collect(
                StringBuilder::new,
                (sb, line) -> {
                  sb.append(line);
                  if (newLine.get()) {
                    sb.append("\n");
                  } else {
                    sb.append(" ");
                  }
                  newLine.set(!newLine.get());
                },
                StringBuilder::append)
            .toString()
            .split("\n"));
  }

  private String extractVariableName(Path path) {
    String fileName = path.getFileName().toString();
    String variableName = fileName.substring(0, fileName.lastIndexOf('.'));
    return variableName;
  }

  public CompressedData compress(Path path) throws IOException {
    CompressedData compressedData = new CompressedData(extractVariableName(path));
    AtomicInteger offset = new AtomicInteger(0);
    AtomicInteger lineNumber = new AtomicInteger(0);

    format(filter(getData(path)))
        .forEach(
            line -> {
              if (compressedData.containsKey(line)) {
                compressedData.addIndex(compressedData.getChunkOffset(line));
              } else {
                compressedData.putData(line, offset.getAndAdd(16));
                compressedData.addIndex(lineNumber.getAndIncrement());
              }
            });

    AtomicInteger v = new AtomicInteger(0);
    compressedData.data.entrySet().stream()
        .sorted(Entry.comparingByValue())
        .map(Entry::getKey)
        .forEach(
            line ->
                Stream.of(line.split(", "))
                    .map(value -> value.replace("0x", ""))
                    .map(value -> value.replace(",", ""))
                    .mapToInt(value -> Integer.parseInt(value, 16))
                    .forEach(
                        value -> {
                          compressedData.values.add(value);
                          System.out.println(v.getAndIncrement()+ ": " + value);
                        }));

    return compressedData;
  }
}
