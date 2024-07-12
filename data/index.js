var currentData;
var devicesData;

$(document).ready(function() {
    refreshData();
    getDevicesData();
    refreshScreen();
});
//
// end of init functions
//

//
// wifi functions
//
$(document).on('click','#wifi-switch', function (){
    var ckb = $("#wifi-switch").is(':checked');
    if(ckb){
        setStationMode();
    }else{
        setAccessPointMode();
    }
});
function setAccessPointMode(){
    $('#wifi-switch').prop('checked', false);
    $('.ap-mode').removeClass('d-none').addClass('d-block');
    $('.station-mode').removeClass('d-block').addClass('d-none');
    $('#ap-address').val(currentData.apaddress);
    $('#ap-password').val('');
    $('#ap-hostname').val(currentData.hostname);
}

function setStationMode(){
    var ssids = JSON.parse(currentData.ssids);
    $('.station-mode').removeClass('d-none').addClass('d-block');
    $('.ap-mode').removeClass('d-block').addClass('d-none');
    $('#ssid').find('option').remove().end();
    $('#ipaddress').html(currentData.ipAddress);
    $('#wifi-switch').prop('checked', true);
    var optionText;
    var color;
    $.each(ssids, function(i,ss) {   
        if(ss.rssi>-60 && ss.rssi<=-50){
            color='green';
        }else if(ss.rssi>-80 && ss.rssi<-60){
            color='yellow';
        }else if( ss.rssi<-80){
            color='red';
        }
        if(ss.enc){
            optionText = ss.ssid + '   ' +ss.rssi + '  ' +'pwd';
        }else{
            optionText = ss.ssid + '   ' +ss.rssi + '  ' +'';
        }
        $('#ssid').append($("<option>",{
                        value: ss.ssid,
                            text:optionText
                        }));
    });
    if(currentData.ssid!=''){
        $('#ssid').val(currentData.ssid);
    }
    $('#wifi-password').val('');
    $('#hostname').val(currentData.hostname);
    
}


function setStationMode1(){
    $.ajax({
        type: "GET",
        async:false,
        url: "/CajalServlet",
        data:{formName:"GetAvailableSSIDS"},
        success: function (result) {
            var ssids = JSON.parse(result);
            $('.station-mode').removeClass('d-none').addClass('d-block');
            $('.ap-mode').removeClass('d-block').addClass('d-none');
            $('#ssid').find('option').remove().end();

            $('#wifi-switch').prop('checked', true);
            
            var color;
            $.each(ssids, function(i,ss) {   
                

                if(ss.rssi>-60 && ss.rssi<=-50){
                    color='green';
                }else if(ss.rssi>-80 && ss.rssi<-60){
                    color='yellow';
                }else if( ss.rssi<-80){
                    color='red';
                }

                $('#ssid').append($("<option></option>")
                            .attr("value", ss.name)
                            .text(ss.name + '   ' +ss.rssi + '  ' + ss.enc)); 
                });
            $('#ssid').val(currentData.ssid);
            $('#wifi-password').val('');
            $('#hostname').val(currentData.hostname);
            
        },
        error: function(data){
            console.log("solo:" + JSON.stringify(data));
            alert("There was an error processing your request:" + JSON.stringify(data));
            return false;
        }
    });
    
}




$(document).on('click','#wifi-config-icon', function (){
    $('.display-module').removeClass('d-block').addClass('d-none');
    $('#wifi-configure').removeClass('d-none').addClass('d-block');
    
    if(currentData.stationmode){
        setStationMode();
    }else{
        setAccessPointMode();
    }
   
});

$(document).on('click','#wifi-configure-cancel', function (){
    refreshScreen();
 });

 $(document).on('click','#wifi-configure-submit', function (){
    var ckb = $("#wifi-switch").is(':checked');
    
    if(ckb){
        var ssid = $('#ssid').val();
        var pass=$('#wifi-password').val();
        var host =  $('#hostname').val();
        if(ssid== undefined || ssid==''){
            alert("Please input SSID");
            return false;
        };
        if(host== undefined || host==''){
            alert("Please input Host");
            return false;
        };
        
        $.ajax({
            type: "POST",
            url: "/CajalServlet",
            data: {formName:'ConfigSTA',ssid:ssid,pass:pass,host:host},
            success: function (result) {
                currentData = JSON.parse(result);
                refreshScreen();
            },
            error: function(data){
                console.log("solo:" + JSON.stringify(data));
                alert("There was an error processing your request:" + JSON.stringify(data));
                return false;
            }
        });
        
    }else{
        var apaddress = $('#ap-address').val();
        var pass=$('#ap-password').val();
        var host =  $('#ap-hostname').val();
        if(apaddress== undefined || apaddress==''){
            alert("Please input AP Address");
            return false;
        };
        if(host== undefined || host==''){
            alert("Please input Host");
            return false;
        };
        
        $.ajax({
            type: "POST",
            url: "/CajalServlet",
            data: {formName:'ConfigAP',apaddress:apaddress,pass:pass,host:host},
            success: function (result) {
                currentData = JSON.parse(result);
                refreshScreen();
            },
            error: function(data){
                console.log("solo:" + JSON.stringify(data));
                alert("There was an error processing your request:" + JSON.stringify(data));
                return false;
            }
        });

    }
    
    alert("Please wait 5 seconds and refresh");

   
   
 
  });
//
// end of wifi functions
//


$(document).on('click','#system-info-icon', function (){
    $('.display-module').removeClass('d-block').addClass('d-none');
    if($('#system-info').hasClass('d-block')){
        $('#system-info').removeClass('d-block').addClass('d-none');
    }else{
        $('#system-info').removeClass('d-none').addClass('d-block');
    }
    if(currentData.internetAvailable){
        $('#internet-time-configure-button').removeClass('d-none').addClass('d-block');
        $('#internet-ping-time').html(currentData.internetPingTime);
    }else{
        $('#internet-time-configure-button').removeClass('d-block').addClass('d-none');
        $('#internet-ping-time').html('');
    }
    if(currentData.latitude==null || currentData.latitude==undefined){
        $('#latitude').val('');
    }else{
        $('#latitude').val(currentData.latitude);
    }

    if(currentData.longitude==null || currentData.longitude==undefined){
        $('#longitude').val('');
    }else{
        $('#longitude').val(currentData.longitude);
    }
});

$(document).on('click','#system-info-cancel', function (){
    refreshScreen();
 });


$(document).on('click','#manual-time-button', function (){
    var date = new Date();
    var day = ("0" + date.getDate()).slice(-2);
    var month = ("0" + (date.getMonth() + 1)).slice(-2);
    var today = date.getFullYear() + "-" + (month) + "-" + (day) +'T'+  date.getHours() + ":"+ date.getMinutes()+":"+date.getSeconds();
    $('#date-input').val(today);
    $('#manual-time-modal').modal('show');
});



$(document).on('click','#manual-time-submit', function (){
    var date = new Date($('#date-input').val());
    if(date.getTime()<10000){
     alert('Please input valid time');
     return false;
    }
   
    var day = date.getDate();
    var month = (date.getMonth() + 1);
    var year = date.getYear()-100;
    var time = "SetTime#" + (day)+"#" +(month) +"#"+ year +"#"+(date.getDay()) + "#"+  date.getHours() + "#"+ date.getMinutes()+"#"+date.getSeconds();    
    $.ajax({
         type: "POST",
         url: "/CajalServlet",
         data:{formName:"ManualSetTime",time:time},
         success: function (result) {
            $('#manual-time-modal').modal('hide');
             currentData = JSON.parse(result);
             refreshScreen();
         },
         error: function(data){
             console.log("solo:" + JSON.stringify(data));
             alert("There was an error processing your request:" + JSON.stringify(data));
             return false;
         }
     });
  });

  $(document).on('click','#internet-time-configure-button', function (){
       $.ajax({
         type: "POST",
         url: "/CajalServlet",
         data:{formName:"SetTimeViaInternet"},
         success: function (result) {
             currentData = JSON.parse(result);
             refreshScreen();
         },
         error: function(data){
             console.log("solo:" + JSON.stringify(data));
             alert("There was an error processing your request:" + JSON.stringify(data));
             return false;
         }
     });
 
  });

  $(document).on('click','#save-gps-button', function (){
    var lat = $('#latitude').val();
    var long = $('#longitude').val();
    if(lat==''){
        alert("Please input the latitude");
        return false;
    }
    if(long==''){
        alert("Please input the longitude");
        return false;
    }
    $.ajax({
      type: "POST",
      url: "/CajalServlet",
      data:{formName:"SetGPS",lat:lat,long:long},
      success: function (result) {
          currentData = JSON.parse(result);
          refreshScreen();
      },
      error: function(data){
          console.log("solo:" + JSON.stringify(data));
          alert("There was an error processing your request:" + JSON.stringify(data));
          return false;
      }
  });
});

$(document).on('click','.show-raw-data', function (){
    var cc = $(this).data('rawdata');
    var rawdata =  decodeURIComponent(cc);
    var device = JSON.parse(rawdata);
    var devicename =  $(this).data("devicename");
    var tabledata='<table class="table table-striped">';
    for(const key in  device ){
        tabledata+='<tr><td>'+key+'</td><td>'+device[key]+'</td></tr>';
    }
     tabledata+='</table>';
    $('#raw-data-table').empty();
    $('#raw-data-table').append(tabledata);

    $('#raw-data-title').html(devicename);
    $('#raw-data-json').empty();
    $('#raw-data-json').append('<code>'+JSON.stringify(device) + '</code>');
    $('#show-raw-data-modal').modal('show');
 });


function getDevicesData(){
    $.ajax({
         type: "GET",
         async:false,
         url: "/CajalServlet",
         data:{formName:"GetDevicesData"},
         success: function (result) {
             devicesData = JSON.parse(result);
             
         },
         error: function(data){
             console.log("solo:" + JSON.stringify(data));
             alert("There was an error processing your request:" + JSON.stringify(data));
             return false;
         }
     });
  }

function refreshData(){
    $.ajax({
         type: "GET",
         async:false,
         url: "/CajalServlet",
         data:{formName:"GetWebData"},
         success: function (result) {
             currentData = JSON.parse(result);
             
         },
         error: function(data){
             console.log("solo:" + JSON.stringify(data));
             alert("There was an error processing your request:" + JSON.stringify(data));
             return false;
         }
     });
  }

function refreshScreen(){ 
    $('.display-module').removeClass('d-block').addClass('d-none');
    
    
    $("#wifi-config-icon").removeClass (function (index, className) {
        return (className.match (/\btext-\S+/g) || []).join(' ');
    });
    $('#host-name').html(currentData.hostname);
    $('#ssid-name').html(currentData.ssid);
    var d = new Date(currentData.secondsTime*1000);
    const minutes = String(d.getMinutes()).padStart(2, '0');
    const seconds = String(d.getSeconds()).padStart(2, '0');
    if(currentData.digitalStablesUpload){
        $('#ds-upload').css('color', 'green');
    }else{
        $('#ds-upload').css('color', 'red');
    }

    var datestring = d.getHours() + ":" + minutes+":"+ seconds + " " + d.getDate() +"/" +(d.getMonth()+1) + "/" + d.getFullYear();
    $('#time').html(datestring);
    
    if(currentData.lora){
        $('#lora-logo').css('color', 'blue');
    }else{
        $('#lora-logo').css('color', 'red');
    }
    
    $('#serial-number').html(currentData.serialnumber);
    $('#temperature').html( currentData.temperature + '&#8451;');

    $("#rtc-bat-volt").html((Math.round(currentData.rtcBatVolt * 100) / 100).toFixed(2));
    if(currentData.rtcBatVolt>=2.7){
        $('#rtc-battery-icon').css("color", "green");
    }else if(currentData.rtcBatVolt>=2.5 && currentData.rtcBatVolt<2.7){
        $('#rtc-battery-icon').css("color", "yellow");
    }else if(currentData.rtcBatVolt<2.5){
        $('#rtc-battery-icon').css("color", "red");
    }
    if(currentData.rssi>-60 && currentData.rssi<=-50){
        $('#wifi-config-icon').addClass('text-sucess');
    }else if(currentData.rssi>-80 && currentData.rssi<-60){
        $('#wifi-config-icon').addClass('text-warning');
    }else if( currentData.rssi<-80){
        $('#wifi-config-icon').addClass('text-danger');
    }

    $('#wifi-info').removeClass('d-none').addClass('d-block');
    var tableData="";
    var device;
    var rawdata;
    for(const key in  devicesData ){
        device = devicesData[key];
        var imagename='';
        if(device.deviceTypeId=='Rosie'){
            imagename='/assets/img/Rosie'
        }else if(device.deviceTypeId.startsWith('Pancho')){
            imagename='/assets/img/Pancho'
        }else if(device.deviceTypeId=='Gloria'){
            imagename='/assets/img/Gloria'
        }
        var modeText='';
        if(device.currentFunctionValue==FUN_1_FLOW){
            modeText='One Flow Sensor';
         }else if(device.currentFunctionValue==FUN_2_FLOW){
            modeText='Two Flow Sensors';
         }else if(device.currentFunctionValue==FUN_1_FLOW_1_TANK){
            modeText='One Flow - One Tank';
         }else if(device.currentFunctionValue==FUN_1_TANK){
            modeText='One Tank Sensor';
         }else if(device.currentFunctionValue==FUN_2_TANK){
            modeText='Two Tank Sensors';
         }
         var  deviceTime='';
        var millis = 1000*device.secondsTime;
        if(millis>0){
            let myDate = new Date(millis  );
            deviceTime = myDate.getFullYear() + "/" + (myDate.getMonth() + 1) + "/" + myDate.getDate() + " " + myDate.getHours() + ":" + myDate.getMinutes() + ":" + myDate.getSeconds()
    
        }else{
            if(device.rtcBatVol<=2.8){
                deviceTime='Replace battery and re set Time';
            }else{
                deviceTime='Time needs to be reset'
            }
        }
        rawdata = encodeURIComponent( JSON.stringify(device));
        tableData+='<div id="'+device.devicename.replace(" ","-")+'" class="card col-12 show-raw-data" data-devicename="'+device.devicename+'" data-rawdata="'+rawdata+'" style="border: black 2px solid;margin:10px;">';
        tableData+='<div class="card-body row d-flex flex-wrap align-items-top" style="padding:0px">';
      
        tableData+='<img class="col-3 " src="'+ imagename+'.svg" width="10" height="90" style="padding-left: 0px;margin-right: -13px;padding-top: 0px;margin-top: -3px;align: top;" />';
        tableData+='<div class="col-9 row" style="margin-left: 0px;">';
        tableData+='<label class="form-label col-12" style="fonty-size:20px;margin:10px">'+device.devicename+'</label>';
        tableData+='<label class="form-label col-12" style="margin-top: -30px;font-size: 12px;padding:0px;">'+deviceTime+'</label>';
        tableData+='</div>';
        tableData+='<div class="row" >';
        tableData+='<label class="form-label col-12" style="margin-top: -5px;font-size: 12px;">Mode : '+modeText+'</label>';
        if(device.loraActive)tableData+='<label class="form-label col-12" style="font-size: 12px;margin-top: -5px;">Lora  : rssi:'+device.rssi+ ' snr:'+device.snr+'</label>';
        else tableData+='<label class="form-label col-12" style="margin-top: -5px;font-size: 12px;">Lora is not active</label>';
        if(device.internetAvailable)tableData+='<label class="form-label col-12" style="font-size: 12px;margin-top: -5px;">WIFI Active, ip: '+device.ipAddress+'</label>';
        else tableData+='<label class="form-label col-12" style="margin-top: -5px;font-size: 12px;">Wifi is not active</label>';
        tableData+='</div>';
        tableData+='<div class="row" style="font-size:20px">';
        var totalVolume, totalVolume2;
        if(device.currentFunctionValue==FUN_1_FLOW){
            tableData+='<label class="form-label col-6">'+device.flow1name +' : '+device.flowrate+' l/s</label>';
            totalVolume=device.totalmilliLitres/1000;
            tableData+='<label class="form-label col-6">Total : '+totalVolume+' l</label>';
         }else if(device.currentFunctionValue==FUN_2_FLOW){
            tableData+='<label class="form-label col-6">'+device.flow1name +' : '+device.flowrate+' l/s</label>';
            totalVolume=device.totalmilliLitres/1000;
            tableData+='<label class="form-label col-6">Total : '+totalVolume+' l</label>';

            tableData+='<label class="form-label col-6">'+device.flow2name +' : '+device.flowrate2+' l/s</label>';
            totalVolume2=device.totalmilliLitres2/1000;
            tableData+='<label class="form-label col-6">Total : '+totalVolume2+' l</label>';

         }else if(device.currentFunctionValue==FUN_1_FLOW_1_TANK){
            tableData+='<label class="form-label col-6">'+device.flow1name +' : '+device.flowrate+' l/s</label>';
            totalVolume=device.totalmilliLitres/1000;
            tableData+='<label class="form-label col-6">Total : '+totalVolume+' l</label>';
            tableData+='<label class="form-label col-1222">'+device.tank1name +'  : '+device.tank1waterLevel +' l  </label>';
           
         }else if(device.currentFunctionValue==FUN_1_TANK){
            tableData+='<label class="form-label col-1222">'+device.tank1name +'  : '+device.tank1waterLevel +' l  </label>';
         }else if(device.currentFunctionValue==FUN_2_TANK){
            tableData+='<label class="form-label col-1222">'+device.tank1name +'  : '+device.tank1waterLevel +' l  </label>';
            tableData+='<label class="form-label col-1222">'+device.tank2name +'  : '+device.tank2waterLevel +' l  </label>';
         }


        tableData+='</div>';
        tableData+='</div>';
        tableData+='</div>';
    }
    $('#workarea').empty();
    $('#workarea').append(tableData);
}