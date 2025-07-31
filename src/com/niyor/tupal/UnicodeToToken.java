
package com.niyor.tupal;

import java.util.List;
import java.util.HashMap;
import java.util.Map;

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

    public String getLiteral(List<String> digits) {
        StringBuilder result = new StringBuilder();
        for (String code : digits) {
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
}

