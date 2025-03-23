function toggleGroup(id, btn) {
    var group = document.getElementById(id);
    if (group.style.display === 'none') {
        group.style.display = '';
        btn.textContent = '▼';
    } else {
        group.style.display = 'none';
        btn.textContent = '►';
    }
}

function toggle(event, id) {
    event.stopPropagation();
    const children = document.querySelectorAll(`[data-parent='${id}']`);
    if (!children.length) return;
    const first = children[0];
    const shouldShow = first.style.display === "none";
    children.forEach(row => {
        row.style.display = shouldShow ? "" : "none";
        // If collapsing, also collapse all children recursively
        if (!shouldShow) {
            const subId = row.getAttribute("data-id");
            const subChildren = document.querySelectorAll(`[data-parent='${subId}']`);
            subChildren.forEach(r => r.style.display = "none");
            const toggleSpan = row.querySelector(".fold-toggle");
            if (toggleSpan) toggleSpan.textContent = "▶";
        }
    });
    // Change the folding symbol
    const toggleSpan = event.target.closest(".fold-toggle");
    if (toggleSpan) {
        toggleSpan.textContent = shouldShow ? "▼" : "▶";
    }
}

function unsubscribe(clientId, topic) {
    fetch("/unsubscribe", {
        "method": "POST",
        "body": JSON.stringify({
            "client_id": clientId,
            "topic": topic
        }),
        "headers": {
            "Content-type": "application/json; charset=UTF-8"
        }
    }).then(response => {
        return response.text().then(body => {
            return {
                "status": response.status,
                "body": body,
                "ok": response.ok
            }
        })
    }).then(result => {
        if (!result.ok) {
            throw new Error("Network response was not ok\n" + result.status + ": " + result.body)
        }
        return result.body
    }).then(body => {
        console.log("Data received:", body)
        window.location.reload()
    }).catch(error => {
        console.error("There was a problem with the fetch operation:", error)
        alert(error)
        window.location.reload()
    })
}

function subscribe(clientId, topic, qoS) {
    fetch("/subscribe", {
        "method": "POST",
        "body": JSON.stringify({
            "client_id": clientId,
            "topic": topic,
            "qos": qoS
        }),
        "headers": {
            "Content-type": "application/json; charset=UTF-8"
        }
    }).then(response => {
        return response.text().then(body => {
            return {
                "status": response.status,
                "body": body,
                "ok": response.ok
            }
        })
    }).then(result => {
        if (!result.ok) {
            throw new Error("Network response was not ok\n" + result.status + ": " + result.body)
        }
        return result.body
    }).then(body => {
        console.log("Data received:", body)
        window.location.reload()
    }).catch(error => {
        console.error("There was a problem with the fetch operation:", error)
        alert(error)
        window.location.reload()
    })
}