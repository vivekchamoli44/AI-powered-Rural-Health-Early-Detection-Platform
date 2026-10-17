
let alertsPollingTimer = null;

async function loadHistory(patientId, isSilent) {
    const container = document.getElementById('history-timeline-box');
    if (!container) return;

    if (!isSilent) {
        container.innerHTML = '<p style="text-align: center; color: var(--text-muted); padding: 15px;">Loading past assessments...</p>';
    }

    try {
        const id = patientId || 2;
        const response = await fetch('/api/assessments/my?patient_id=' + id);
        const data = await response.json();
        const list = data.assessments || [];

        // Auto-poll in background every 8 seconds for real-time ASHA alerts
        // (patient.html is a dedicated page now, so no visibility check is needed)
        if (!alertsPollingTimer) {
            alertsPollingTimer = setInterval(function () {
                loadHistory(id, true);
            }, 8000);
        }

        if (list.length === 0) {
            container.innerHTML = '<div style="text-align: center; padding: 20px; color: var(--text-muted); background: #f8fafc; border-radius: 8px; border: 1px dashed var(--border);"><p>No previous assessments found. Click <strong>Start New Assessment</strong> above.</p></div>';
            renderAlertsSidebar([]);
            return;
        }

        // Render ASHA worker alerts in the patient sidebar
        renderAlertsSidebar(list);

        container.innerHTML = '';
        list.forEach(function (item) {
            let badgeClass = 'badge-low';
            if (item.risk_level === 'Critical') badgeClass = 'badge-critical';
            else if (item.risk_level === 'High') badgeClass = 'badge-high';
            else if (item.risk_level === 'Moderate') badgeClass = 'badge-moderate';

            const card = document.createElement('div');
            card.className = 'card';
            card.style.display = 'flex';
            card.style.justifyContent = 'space-between';
            card.style.alignItems = 'center';
            card.style.marginBottom = '12px';

            card.innerHTML = `
                <div>
                    <div style="display: flex; align-items: center; gap: 8px; margin-bottom: 4px;">
                        <strong>Assessment #${item.id}</strong>
                        <span class="badge ${badgeClass}">${item.risk_level} (${item.risk_score}/100)</span>
                        ${item.alert_sent ? '<span class="alert-flag">🚨 Alert Dispatched</span>' : ''}
                    </div>
                    <div style="font-size: 13px; color: var(--text-muted); margin-bottom: 6px;">
                        Date: ${item.created_at || 'Recent'} | Specialist: <strong style="color: var(--primary);">${item.recommended_doctor}</strong>
                    </div>
                    <div style="font-size: 13px; color: var(--text-muted);">
                        Reported Symptoms: <strong>${item.symptoms_text || 'None reported'}</strong>
                    </div>
                </div>
                <div>
                    <button class="btn btn-outline btn-sm view-past-btn">View Report</button>
                </div>
            `;

            card.querySelector('.view-past-btn').addEventListener('click', function () {
                if (window.AiAssessmentModule) {
                    window.AiAssessmentModule.renderReport(item, true);
                }
            });

            container.appendChild(card);
        });

    } catch (err) {
        container.innerHTML = '<p style="color: #ef4444; padding: 10px;">Failed to load history: ' + err.message + '</p>';
    }
}
