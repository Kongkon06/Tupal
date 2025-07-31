
# 🐍 Tupal – An Assamese Scripting Language

**Tupal** is a small scripting language for the Assamese-speaking community.  
Inspired by [Lox](https://craftinginterpreters.com/), it brings a readable, expressive syntax to native Assamese text.

> ✨ Write code in your own language. Compile and run it with a custom interpreter written in Java.

---

## 💡 About

Tupal is:
- 🧠 **Interpreted** – implemented in Java, based on Robert Nystrom's Lox.
- 🗣️ **Localized** – keywords and syntax are in **Assamese Unicode**.
- 🪶 **Lightweight** – built for learning, prototyping, and educational purposes.

---

## 🧪 Example

A sample Tupal program:

```assamese
চল ক = ০;
চল সাময়িক;
যদিয়া (চল খ = ১; ক < ১০০০০; খ = সাময়িক + খ) {
  প্ৰকাশ ক;
  সাময়িক = ক;
  ক = খ;
}
