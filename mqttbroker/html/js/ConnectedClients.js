function attach(tableSel, url) {
    const table = document.getElementById(tableSel);
    if (!table) return console.error("Table not found:", tableSel);
    const cols = table.tHead?.rows[0]?.cells.length ?? 0;
    const tbody = table.tBodies[0] || table.appendChild(document.createElement("tbody"));
    const coll = new Intl.Collator(undefined, {
        usage: 'sort',
        sensitivity: 'base', // ignore case/accents
        ignorePunctuation: true,
        numeric: true // natural numbers inside names (optional)
    });
    // e.g., columns 0 and 7 contain HTML
    const HTML_COLS = new Set([0, 2, 6]); // zero-based indexes
    function renderCell(td, value, colIndex) {
        if (HTML_COLS.has(colIndex)) {
            const html = String(value ?? "");
            td.innerHTML = window.DOMPurify ? DOMPurify.sanitize(html
                /* , {
                   ALLOWED_TAGS: ['b','i','em','strong','a'],
                   ALLOWED_ATTR: ['href','title','target']
                 } */
            ) : html; // trust only if you must
        } else {
            td.textContent = value ?? "";
        }
    }

    function makeRow(arr) {
        const tr = document.createElement("tr");
        tr.dataset.key = String(Array.isArray(arr) ? arr[0] ?? "" : "");
        for (let i = 0; i < cols; i++) {
            const td = document.createElement("td");
            renderCell(td, arr[i], i);
            tr.appendChild(td);
        }
        return tr;
    }
    const norm = s => String(s ?? "").replace(/\s+/g, " ").trim().normalize("NFKC");

    function compareKeys(a, b) {
        const A = norm(a);
        const B = norm(b);
        if (A === B) {
            return 0;
        }
        // Locale-aware compare (numbers, accents, case handled by collator options)
        const c = coll.compare(A, B);
        if (c !== 0) {
            return c;
        }
        // Deterministic tiebreaker (rare but good hygiene)
        return A < B ? -1 : 1;
    }
    const insertSorted = (tr) => {
        const key = tr.cells[0]?.innerText;
        for (const row of tbody.rows) {
            const rowKey = row.cells[0]?.innerText ?? "";
            const cmp = compareKeys(key, rowKey);
            if (cmp < 0) {
                return tbody.insertBefore(tr, row);
            }
            if (cmp === 0) {
                return tbody.replaceChild(tr, row); // update existing
            }
        }
        tbody.appendChild(tr);
    };
    const es = new EventSource(url); // <-- your SSE endpoint
    const connectHandle = (ev) => {
        try {
            const arr = JSON.parse(ev.data);
            if (Array.isArray(arr)) insertSorted(makeRow(arr));
        } catch (e) {
            console.error("Bad SSE payload:", e, ev.data);
        }
    };
    const disconnectHandle = (ev) => {
        let needle = ev.data;
        try {
            const parsed = JSON.parse(ev.data);
            needle = parsed.key ?? parsed.s ?? parsed; // pick your field name
        } catch (e) {
            console.log("Not a JSON: ", e, ev.data);
        }
        if (typeof needle !== "string" || !needle) return;
        for (const tbody of table.tBodies) {
            for (const row of Array.from(tbody.rows)) {
                const cell0 = row.cells[0];
                if (!cell0) {
                    continue;
                }
                // Prefer a precomputed sort key; fallback to user-visible text
                const hay = (cell0.dataset && cell0.dataset.sortKey) || cell0.innerText || cell0.textContent || "";
                if (hay.toLocaleLowerCase() === needle.toLocaleLowerCase()) {
                    row.remove();
                    return; // remove only first match; drop this line to remove all
                }
            }
        }
    }
    const errorHandle = (ev) => {
        for (const tbody of table.tBodies) {
            tbody.replaceChildren();
        }
    }
    es.addEventListener("connect", connectHandle);
    es.addEventListener("disconnect", disconnectHandle)
    es.addEventListener("error", errorHandle);
}
