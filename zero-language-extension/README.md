# Zero Language Support for VS Code

This extension provides syntax highlighting for the Zero programming language.

## Features

- Syntax highlighting for .z files
- Code completion for keywords, types, and standard library functions
- Basic language configuration (comments, brackets, etc.)

## Installation

### From VSIX File
1. Open VS Code
2. Go to Extensions (Ctrl+Shift+X)
3. Click on the three dots (...) in the top right corner
4. Select "Install from VSIX..."
5. Navigate to the `zero-language-extension` directory and select the extension file

### Development Mode
1. Open the `zero-language-extension` directory in VS Code
2. Press F5 to launch the extension in a new VS Code window

## Building the VSIX File

To build the extension into a VSIX file, follow these steps:

1. Install the VS Code Extension Manager (vsce):
   ```bash
   npm install -g vsce
   ```

2. Navigate to the extension directory:
   ```bash
   cd zero-language-extension
   ```

3. Install dependencies:
   ```bash
   npm install
   ```

4. Build the extension:
   ```bash
   npm run build
   ```

This will compile the TypeScript code and generate a `.vsix` file in the extension directory that you can install in VS Code.

## Usage

The extension will automatically detect .z files and provide:

1. **Syntax highlighting** for all language elements
2. **Code completion** for:
   - Keywords: `func`, `var`, `return`, `if`, `else`, `while`, `include`, `typedef`, `new`, `null`
   - Types: `Int`, `Bool`, `string`, `object`
   - Standard library functions: `print`, `swap`, `__print`

## Language Features

Based on the Zero language syntax, the extension highlights:

- Keywords: `func`, `var`, `return`, `if`, `else`, `while`, `include`, `typedef`, `new`, `null`
- Types: `Int`, `Bool`, `string`, `object`
- Strings: Double-quoted strings
- Numbers: Integer and floating-point numbers
- Booleans: `true`, `false`
- Operators: Arithmetic, assignment, comparison, logical
- Punctuation: Semicolons, commas, brackets, etc.
- Functions: Function definitions
- Types: Type definitions
- Variables: Variable names

## Future Enhancements

- Code completion
- Go to definition
- Find references
- Syntax error checking
- Debugging support
