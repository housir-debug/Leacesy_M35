/* ========== Header Component ========== */
const headerHTML = `
<header>
    <div class="header-brand">
        <div class="logo-container">
            <img src="/icon/leacesyicon.png" alt="Logo" class="logo-img">
            <div class="logo-pulse"></div>
        </div>
        <div class="brand-text">
            <h1>Instrument Control</h1>
            <div class="brand-status">
                <span class="status-indicator-small"></span>
                <span id="header-status-text">Device Service Online...</span>
            </div>
        </div>
    </div>
    <nav>
        <a href="/index.html" id="nav-console">Console</a>
        <a href="/channels.html" id="nav-channels">Dashboard</a>
        <a href="/import.html" id="nav-import">Import</a>
    </nav>
</header>
`;

function loadHeader(containerId, page) {
    const container = document.getElementById(containerId);
    if (!container) return;
    
    container.innerHTML = headerHTML;
    const navLink = document.getElementById(`nav-${page}`);
    if (navLink) {
        navLink.classList.add('active');
    }
}

window.sharedComponents = {
    loadHeader
};
