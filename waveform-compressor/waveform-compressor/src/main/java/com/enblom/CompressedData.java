package com.enblom;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map.Entry;

public class CompressedData {

  private final String name;
  List<Integer> values = new ArrayList<>();
  HashMap<String, Integer> data = new HashMap<>();
  List<Integer> index = new ArrayList<>();

  public CompressedData(String name) {
    this.name = name;
  }

  boolean containsKey(String key) {
    return data.containsKey(key);
  }

  int getChunkOffset(String key) {
    return data.get(key);
  }

  void putData(String key, int lineNumber) {
    data.put(key, lineNumber);
  }

  void addIndex(int offsetValue) {
    index.add(offsetValue);
  }

  int getValue(int address) {
    int base = address & 0xFFF0;
    int lineOffset = address - base;
//    System.out.println();
//    System.out.println("address: " + address);
//    System.out.println("base: " + base);
//    System.out.println("lineOffset: " + lineOffset);
//    System.out.println("index: " + base / 16);
//    System.out.println("base offset: " + index.get(base / 16));
//    System.out.println();
    return values.get(index.get(base / 16) + lineOffset);
  }

  @Override
  public String toString() {
    StringBuilder sb = new StringBuilder();

    sb.append("// This file is auto-generated. Do not edit.\n");
    sb.append("// Value lookup follows this pattern:\n");
    sb.append("// base = input & 0xfff0\n");

    sb.append("reg8 WaveformGenerator::").append(name).append("_data[] = {\n");

    data.entrySet().stream()
        .sorted(Entry.comparingByValue())
        .forEach(set -> sb.append("\t").append(set.getKey()).append("\n"));

    sb.append("};\n\n");

    sb.append("reg8 WaveformGenerator::").append(name).append("_index[256][1] = {\n");
    index.stream()
        .sorted()
        .forEach(
            offsetValue ->
                sb.append(String.format("\t%d,\n", offsetValue)));

    sb.append("};\n\n");

    sb.append("reg8 WaveformGenerator::")
        .append(name)
        .append("_size = ")
        .append(values.size())
        .append(";\n");

    return sb.toString();
  }
}
