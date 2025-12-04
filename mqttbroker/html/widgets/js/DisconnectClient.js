function disconnectClient(clientId) {
    if (typeof showSpinner === 'function') {
        showSpinner?.()
    }
    fetch("/disconnect", {
        "method": "POST",
        "body": JSON.stringify({
            "client_id": clientId
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
    }).catch(error => {
        console.error("There was a problem with the fetch operation:", error)
        alert(error)
    }).finally(() => {
        if (typeof hideSpinner === 'function') {
            hideSpinner?.()
        }
    })
}
