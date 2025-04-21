const overlay = document.getElementById('spinner-overlay')

function showSpinner() {
    var spinnerState = window.sessionStorage.getItem('spinnerState');
    if (spinnerState === null || spinnerState === 'inactive') {
        console.log("Show Spinner")
        window.sessionStorage.setItem('spinnerState', 'active')
        overlay.classList.add('active')
    }
}

function hideSpinner() {
    var spinnerState = window.sessionStorage.getItem('spinnerState');
    if (spinnerState === null || spinnerState === 'active') {
        console.log("Hide Spinner")
        window.sessionStorage.setItem('spinnerState', 'inactive')
        overlay.classList.remove('active')
    }
}

showSpinner()
