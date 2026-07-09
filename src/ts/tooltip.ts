const TOOLTIP_DELAY_MS = 1000;
const TOOLTIP_GAP_PX = 12;
const TOOLTIP_POINT_SIZE_PX = 6;

function positionTooltip(tooltip: HTMLDivElement, anchor: Element) {
    const anchorRect = anchor.getBoundingClientRect();
    const tooltipRect = tooltip.getBoundingClientRect();
    tooltip.style.left = `${
        anchorRect.left + anchorRect.width / 2 - tooltipRect.width / 2
    }px`;
    tooltip.style.top = `${
        anchorRect.top -
        tooltipRect.height -
        TOOLTIP_GAP_PX -
        TOOLTIP_POINT_SIZE_PX
    }px`;
}

function createPoint(): HTMLDivElement {
    const point = document.createElement("div");
    point.className =
        "absolute top-full left-1/2 border-t-stone-700 dark:border-t-orange-100";
    point.style.marginLeft = `-${TOOLTIP_POINT_SIZE_PX}px`;
    point.style.borderLeft = `${TOOLTIP_POINT_SIZE_PX}px solid transparent`;
    point.style.borderRight = `${TOOLTIP_POINT_SIZE_PX}px solid transparent`;
    point.style.borderTopWidth = `${TOOLTIP_POINT_SIZE_PX}px`;
    point.style.borderTopStyle = "solid";
    return point;
}

export function attachTooltip(el: Element, text: string) {
    let showTimer: number | undefined;
    let tooltip: HTMLDivElement | null = null;

    function show() {
        tooltip = document.createElement("div");
        tooltip.textContent = text;
        tooltip.className =
            "fixed z-50 px-3 py-1.5 rounded text-md whitespace-nowrap pointer-events-none " +
            "bg-stone-700 text-orange-100 dark:bg-orange-100 dark:text-stone-800";
        tooltip.appendChild(createPoint());
        document.body.appendChild(tooltip);
        positionTooltip(tooltip, el);
    }

    function hide() {
        window.clearTimeout(showTimer);
        tooltip?.remove();
        tooltip = null;
    }

    el.addEventListener("mouseenter", () => {
        showTimer = window.setTimeout(show, TOOLTIP_DELAY_MS);
    });
    el.addEventListener("mouseleave", hide);
}
