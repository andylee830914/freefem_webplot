var draw = SVG().addTo('#new_plot');

//output btn for 2d SVG
$('#2dsvg').click(function (e) {
    text = $('#new_plot > svg')[0].outerHTML;
    var file = new Blob([text], { type: 'image/svg+xml' });
    var url = URL.createObjectURL(file);
    // var w = window.open(url, 'test.svg');
    this.download = 'output-' + now_file +'.svg';
    this.href = url;
});

//output btn for 2D PNG
$('#2dpng').click(function () {
    text = $('#new_plot > svg')[0].outerHTML;
    var file = new Blob([text], { type: 'image/svg+xml' });
    var canvas = document.getElementById('temp_canvas');
    var box = draw.viewbox();
    // var object  = this;
    // this.download = 'test.png';

    var w1 = window.open("", 'test.png'); // to prevent browser block popup window

    $(canvas).attr('width', 1024 * box.width / box.height).attr('height', '1024');
    var ctx = canvas.getContext('2d');
    var img = new Image();
    img.src = URL.createObjectURL(file);
    img.onload = function () {
        ctx.drawImage(img, 0, 0, 1024 * box.width / box.height, 1024);
        var imgURI = canvas.toDataURL('image/png')
        var blob = dataURItoBlob(imgURI)
        var url = URL.createObjectURL(blob);
        w1.location.href = url;
        // console.log(object);
        // object.href = url;
        ctx.clearRect(0, 0, canvas.width, canvas.height)
        $(canvas).attr('width', '0').attr('height', '0');
    };
});

// 2D Draw
function mydraw2d() {
    var max_x = basic_data.bounds[1][0] - basic_data.bounds[0][0];
    var max_y = basic_data.bounds[1][1] - basic_data.bounds[0][1];
    var max = Math.max(max_x,max_y);
    // var max = max_y;
    const sc = 500 / max;
    const w = 1 * sc;
    const h = -1 * sc;
    try {
        draw.clear()
    } catch (error) {
        console.log(error);
    }

    $("#new_plot > svg").removeAttr('height');


    if ($("#level").is(':checked') && $("#colorbar").is(':checked')) {
        draw_viewbox_w_scale = 0.02 * max_y * 10;
        draw_colorbox = true;
    } else {
        draw_viewbox_w_scale = 0;
        draw_colorbox = false;

    }

    draw.viewbox({
        x: (-0.05 * max_x + basic_data.bounds[0][0]) * sc
        , y: (-0.05 * max_y) * sc
        , width: (1.1 * max_x + draw_viewbox_w_scale) * sc,
        height: (1.1 * max_y) * sc
    })

    var gradient = draw.gradient('linear', function (add) {
        add.stop(0, cprofile[0])
        add.stop(0.25, cprofile[1])
        add.stop(0.50, cprofile[2])
        add.stop(0.75, cprofile[3])
        add.stop(1, cprofile[4])

    })


    var color = [
        new SVG.Color(cprofile[0]).to(cprofile[1]),
        new SVG.Color(cprofile[1]).to(cprofile[2]),
        new SVG.Color(cprofile[2]).to(cprofile[3]),
        new SVG.Color(cprofile[3]).to(cprofile[4])
    ]


    if ($("#mesh").is(':checked')) {
        mesh_data.forEach((e, i) => {
            var polyline = draw.polygon([[e[0].x * w, e[0].y * h - h], [e[1].x * w, e[1].y * h - h], [e[2].x * w, e[2].y * h - h]])
            polyline.fill('none').stroke({ color: '#000000',width: 1 / 500 * sc })
            if ($("#tri_index").is(':checked')) {
                var text = draw.plain(i).attr('x', (e[0].x + e[1].x + e[2].x) / 3 * w).attr('y', (e[0].y + e[1].y + e[2].y) / 3 * h - h)
                    .font({
                        size: 0.02 * sc + "px",
                        anchor: 'middle'
                    })
            }
        });

        if ($("#ver_index").is(':checked') && typeof vertex_data !== 'undefined') {
            vertex_data.forEach((e, i) => {
                var circle = draw.circle(1 / 100 * sc).attr('cx', e.x * w).attr('cy', e.y * h - h);
                var text = draw.plain(i).attr('x', (e.x - 0.015) * w).attr('y', (e.y + 0.01 - 1) * h)
                    .font({
                        size: 0.02 * sc + "px",
                        anchor: 'middle',
                        fill: '#0000FF'
                    })
            })

        }
    } else {
        edge_data.forEach((e, i) => {
            var line = draw.line(e.vertices[0].x * w, e.vertices[0].y * h - h, e.vertices[1].x * w, e.vertices[1].y * h - h)
                .stroke({ width: 1 / 500 * sc });
        })
    }




    if ($("#edg_label").is(':checked') && typeof edge_data !== 'undefined') {
        edge_data.forEach((e, i) => {
            var line = draw.line(e.vertices[0].x * w, e.vertices[0].y * h - h, e.vertices[1].x * w, e.vertices[1].y * h - h)
                .stroke({ width: 1 / 500 * sc }).stroke({ color: '#f06' });
            var text = draw.plain(e.label).attr('x', (e.vertices[0].x + e.vertices[1].x) / 2 * w - 0.015 * sc).attr('y', (e.vertices[0].y + e.vertices[1].y) / 2 * h - h)
                .font({
                    size: 0.02 * sc + "px",
                    anchor: 'middle',
                    fill: '#f06'
                })
        })

    }

    if ($("#level").is(':checked') && typeof mesh_data !== 'undefined') {
        if (draw_colorbox) {
            var rect = draw.rect(max_y * sc, 0.02 * max_y * sc).fill(gradient)
                .attr('x', (basic_data.bounds[1][0] + draw_viewbox_w_scale - 0.11 * max_y) * w)
                .attr('y', (basic_data.bounds[1][1] - basic_data.bounds[0][1]-0.02*max_y) * sc)
                .rotate(270, (basic_data.bounds[1][0] + draw_viewbox_w_scale - 0.11 * max_y) * w, (basic_data.bounds[1][1] - basic_data.bounds[0][1]) * sc)
        }


        nlevel = 20;
        vlevel = [];
        eps = 1e-10;
        dl = (minmax_data[1].u - minmax_data[0].u) / nlevel;
        if (dl == 0) {
            
            return;
        }
        // vlevel.push(minmax_data[0].u);
        for (let i = 0; i <= nlevel; i++) {
            vlevel.push(minmax_data[0].u + i * dl);
        }

        if (vlevel[0] > vlevel[1]) {
            vlevel[0] = vlevel[0] - eps;
            vlevel[nlevel] = vlevel[nlevel] + eps;
        } else {
            vlevel[0] = vlevel[0] + eps;
            vlevel[nlevel] = vlevel[nlevel] - eps;
        }


        for (let vi = 0; vi < vlevel.length; vi++) {
            const ve = vlevel[vi];
            text_space = max_y / nlevel;
            cr = 4 * vi / nlevel;
            ci = Math.floor(cr);
            if (ci == 4) {
                ci = ci - 1;
            }
            if ((vi == 0 || vi == vlevel.length - 1 || vi % 2 == 0) && draw_colorbox) {
                var text = draw.plain(vlevel[vi].toExponential(3)).attr('x', (basic_data.bounds[1][0] + draw_viewbox_w_scale-0.03*max_y) * w).attr('y', (basic_data.bounds[1][1] - text_space * (vlevel.length - vi - 1)) * h - h)
                    .font({
                        size: 0.03 * sc + "px",
                        anchor: 'middle',
                        fill: color[ci].at(cr - ci).toHex()
                    });
            }

            for (let i = 0; i < mesh_data.length; i++) {
                const e = mesh_data[i];
                level_line = [];
                if ((ve >= e[0].u && ve <= e[1].u) || (ve <= e[0].u && ve >= e[1].u)) {
                    //at 1st edge of the element
                    rate = (ve - e[0].u) / (e[1].u - e[0].u)
                    level_line.push([(rate * e[1].x + (1 - rate) * e[0].x) * w, (rate * e[1].y + (1 - rate) * e[0].y) * h - h])
                }
                if ((ve >= e[1].u && ve <= e[2].u) || (ve <= e[1].u && ve >= e[2].u)) {
                    //at 2nd edge of the element
                    rate = (ve - e[1].u) / (e[2].u - e[1].u)
                    level_line.push([(rate * e[2].x + (1 - rate) * e[1].x) * w, (rate * e[2].y + (1 - rate) * e[1].y) * h - h])
                }
                if ((ve >= e[2].u && ve <= e[0].u) || (ve <= e[2].u && ve >= e[0].u)) {
                    //at 3rd edge of the element
                    rate = (ve - e[2].u) / (e[0].u - e[2].u)
                    level_line.push([(rate * e[0].x + (1 - rate) * e[2].x) * w, (rate * e[0].y + (1 - rate) * e[2].y) * h - h])
                }

                draw.polyline(level_line).fill('none').stroke({ width: 1 / 250 * sc }).stroke({ color: color[ci].at(cr - ci).toHex() });
            }
        }
    }



}

