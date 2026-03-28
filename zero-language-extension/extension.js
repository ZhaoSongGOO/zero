"use strict";
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || function (mod) {
    if (mod && mod.__esModule) return mod;
    var result = {};
    if (mod != null) for (var k in mod) if (k !== "default" && Object.prototype.hasOwnProperty.call(mod, k)) __createBinding(result, mod, k);
    __setModuleDefault(result, mod);
    return result;
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.deactivate = exports.activate = void 0;
const vscode = __importStar(require("vscode"));
// Language keywords
const keywords = [
    'func', 'var', 'return', 'if', 'else', 'while', 'include', 'typedef', 'new', 'null'
];
// Language types
const types = [
    'int', 'bool', 'string', 'object'
];
// Standard library functions
const stdFunctions = [
    'print', 'swap', '__print'
];
// Completion item provider class
class ZeroCompletionItemProvider {
    provideCompletionItems(document, position, token, context) {
        const items = [];
        // Add keywords
        keywords.forEach(keyword => {
            const item = new vscode.CompletionItem(keyword, vscode.CompletionItemKind.Keyword);
            item.insertText = keyword;
            items.push(item);
        });
        // Add types
        types.forEach(type => {
            const item = new vscode.CompletionItem(type, vscode.CompletionItemKind.Class);
            item.insertText = type;
            items.push(item);
        });
        // Add standard functions
        stdFunctions.forEach(func => {
            const item = new vscode.CompletionItem(func, vscode.CompletionItemKind.Function);
            item.insertText = func;
            items.push(item);
        });
        return items;
    }
}
function activate(context) {
    console.log('Zero language extension activated');
    // Register completion item provider
    const providerDisposable = vscode.languages.registerCompletionItemProvider('zero', new ZeroCompletionItemProvider());
    context.subscriptions.push(providerDisposable);
}
exports.activate = activate;
function deactivate() { }
exports.deactivate = deactivate;
