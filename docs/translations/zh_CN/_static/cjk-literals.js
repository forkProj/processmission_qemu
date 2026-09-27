/* SPDX-FileCopyrightText: Copyright 2026 Process Mission
 * SPDX-License-Identifier: MIT
 */

// RST diagrams use two columns for wide characters. Font fallback alone
// does not preserve those columns when Latin and Chinese fonts differ.
document.addEventListener("DOMContentLoaded", () => {
    const wide = /\p{Script=Han}|[\u3000-\u303f\uff01-\uff60\uffe0-\uffe6]/u;
    document.querySelectorAll(".translated-prose pre, pre.translated-prose").forEach(pre => {
        const walker = document.createTreeWalker(pre, NodeFilter.SHOW_TEXT);
        const texts = [];
        for (let node = walker.nextNode(); node; node = walker.nextNode()) {
            if (wide.test(node.data)) texts.push(node);
        }
        texts.forEach(node => {
            const fragment = document.createDocumentFragment();
            let plain = "";
            for (const character of node.data) {
                if (wide.test(character)) {
                    if (plain) fragment.appendChild(document.createTextNode(plain));
                    plain = "";
                    const cell = document.createElement("span");
                    cell.className = "cjk-cell";
                    cell.textContent = character;
                    fragment.appendChild(cell);
                } else {
                    plain += character;
                }
            }
            if (plain) fragment.appendChild(document.createTextNode(plain));
            node.replaceWith(fragment);
        });
    });
});
