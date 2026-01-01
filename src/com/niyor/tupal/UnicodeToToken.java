package com.niyor.tupal;

import java.util.List;
import java.util.HashMap;
import java.util.Map;
import java.util.ArrayList;

class UnicodeToToken {
    private static final Map<String, String> assameseNumber;

    static {
        assameseNumber = new HashMap<>();
        assameseNumber.put("09E7", "1");
        assameseNumber.put("09E8", "2");
        assameseNumber.put("09E9", "3");
        assameseNumber.put("09EA", "4");
        assameseNumber.put("09EB", "5");
        assameseNumber.put("09EC", "6");
        assameseNumber.put("09ED", "7");
        assameseNumber.put("09EE", "8");
        assameseNumber.put("09EF", "9");
        assameseNumber.put("09E6", "0");
    }

    public String getLiteral(String digits) {
        String[] digitsList = digits.split(" ");
        StringBuilder result = new StringBuilder();
        for (String code : digitsList) {
            String mapped = assameseNumber.get(code);
            if (mapped != null) {
                result.append(mapped);
            } else {
                result.append("?"); // fallback for unrecognized digit
            }
        }
        return result.toString();
    }

    public static String hexToAssamese(String hexString) {
    StringBuilder result = new StringBuilder();
    String[] codes = hexString.trim().split("\\s+");
    for (String hex : codes) {
        int codePoint = Integer.parseInt(hex, 16);
        result.append((char) codePoint);
    }
    return result.toString();
}
    public static List<String> StringToUnicode(String file) {
    List<String> words = new ArrayList<>();
    StringBuilder word = new StringBuilder();

    for (char lexeme : file.toCharArray()) {
        int code = lexeme;
        String hex = String.format("%04X", code);

        // All delimiters, including "
        if (lexeme == ';' || lexeme == '\n' ||
            lexeme == '(' || lexeme == ')' ||
            lexeme == '{' || lexeme == '}' ||
            lexeme == '"' || lexeme == ',' ||
            lexeme == '.') {

            if (word.length() > 0) {
                words.add(word.toString().trim());
                word.setLength(0);
            }
            words.add(hex);
        }
        else if (lexeme == ' ') {
            if (word.length() > 0) {
                words.add(word.toString().trim());
                word.setLength(0);
            }
        }
        else {
            word.append(hex).append(" ");
        }
    }

    if (word.length() > 0) {
        words.add(word.toString().trim());
    }

    return words;
    }
    public static String fromUnicodeHex(String hexInput) {
        String[] hexCodes = hexInput.trim().split("\\s+");
        StringBuilder result = new StringBuilder();

        for (String hex : hexCodes) {
            try {
                int codePoint = Integer.parseInt(hex, 16);
                result.append(Character.toChars(codePoint));
            } catch (NumberFormatException e) {
                System.err.println("Invalid hex: " + hex);
            }
        }

        return result.toString();
    }
}
