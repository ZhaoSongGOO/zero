import * as vscode from 'vscode';

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
class ZeroCompletionItemProvider implements vscode.CompletionItemProvider {
    provideCompletionItems(
        document: vscode.TextDocument,
        position: vscode.Position,
        token: vscode.CancellationToken,
        context: vscode.CompletionContext
    ): vscode.ProviderResult<vscode.CompletionItem[] | vscode.CompletionList> {
        const items: vscode.CompletionItem[] = [];

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

export function activate(context: vscode.ExtensionContext) {
    console.log('Zero language extension activated');

    // Register completion item provider
    const providerDisposable = vscode.languages.registerCompletionItemProvider(
        'zero',
        new ZeroCompletionItemProvider()
    );

    context.subscriptions.push(providerDisposable);
}

export function deactivate() {}
