function WellURF = readURFs(filename, opt)
% WellURF = readURFs(filename, opt)
% This function reads the *.urfs files which is the result of the gather 
% process of the NPSAT 
% For each streamline this function solves the Advection Dispersion Equation
% using the options defined in opt. 
% Using the options structure one can pass parameters to the ComputeURF 
% function that is called to simulate the 1D ADE 
% The paramteres that can pass are: 
%   alpha and beta parameters that control the longitudinal dispersivity
%   Lmin The streamline length that if a streamline has greater length than 
%   Lmin then the numerical solution is used. Otherwise the analytical 
%   solution is used to avoid numerical dispersion.

    topt.dx = 20; % [m]
    topt.dt = 1; % [years]
    topt.Ttime = 200; % [years]
    topt.Lmin = 200; %[m]
    topt.mult = 1000000;
    if ~isempty(opt)
        topt.aL.alpha = opt.alpha;
        topt.aL.beta = opt.beta;
        if isfield(opt, 'Lmin')
            topt.Lmin = opt.Lmin;
        end
        if isfield(opt, 'Ttime')
            topt.Ttime = opt.Ttime;
        end
        if isfield(opt, 'Agemin')
            topt.Agemin = opt.Agemin;
        end
        if isfield(opt, 'mult')
            topt.mult = opt.mult;
        end
        
    end


    fid = fopen(filename,'r');
    if fid < 0
        WellURF = [];
        return;
    end
    cnter = 1;

    %clf
    %hold on
    % Set the true mode to true to compute the URFS
    true_mode = true;
    
    if true_mode
        WellURF = allocate_space([]);
    else
        WellURF = [];
    end
    while 1
        temp = fgetl(fid);
        if temp == -1
            break;
        end
        C = textscan(temp,'%d',4);
        E_id = C{1}(1);
        S_id = C{1}(2);
        ex_r = C{1}(3);
        fprintf('%d %d\n',[E_id S_id])
        Np = C{1}(4);
        C = fscanf(fid,'%f',Np*4);
        temp = fgetl(fid);
        C = reshape(C,4,Np)';
        C(:,4) = C(:,4)./topt.mult;
        if Np == 1
            continue
        end
        %plot3(C(:,1),C(:,2),C(:,3),'.')
        if true_mode
            id_rmv = sqrt(sum(C(:,1:2).^2,2)) < 10;
            C(id_rmv,:) = [];
            %if sqrt(sum(C(:,1:3).^2)) < 0.1
            %    C(1,:) = [];
            %    warning('The first point of streamline is (0 0 0) and will be removed')
            %end
            [urf, ~] = ComputeURF(C(:,1:3), C(:,4), topt);
            WellURF(cnter,1).Eid = E_id;
            WellURF(cnter,1).Sid = S_id;
            WellURF(cnter,1).Exit = ex_r;
            WellURF(cnter,1).p_cds = C(1,1:3);
            WellURF(cnter,1).v_cds = C(1,4);
            WellURF(cnter,1).p_lnd = C(end,1:3);
            WellURF(cnter,1).v_lnd = C(end,4);
            L = cumsum(sqrt(sum((C(2:end,1:3) - C(1:end-1,1:3)).^2,2)));
            WellURF(cnter,1).L = L(end);
            WellURF(cnter,1).Age = sum(diff([0;L])./C(1:end-1,4))/365;
            % WellURF(cnter,1).v_eff = v_eff(1);
            % WellURF(cnter,1).v_m = v_eff(2);
            WellURF(cnter,1).URF = urf;
            cnter = cnter + 1;
            if cnter > size(WellURF,1)
                WellURF = allocate_space(WellURF);
            end
        else
            dst = sqrt(sum((C(1:end-1,1:3)-C(2:end,1:3)).^2, 2));
            cumsumdst = cumsum(dst);
            v = sum(C(1:end-1,4)+C(2:end,4),2)/2;
            if cumsumdst(end) < 50
                 WellURF(cnter,:) = [C(end,1:3)  sum(dst./v)]; % cumsumdst(end)
                %plot3(C(:,1), C(:,2), C(:,3),'.-')
                %drawnow
               % stop = true;
                cnter = cnter + 1;
            end
            
        end

        
        
    end
    fclose(fid);
    WellURF(cnter:end,:) = [];
end

function well = allocate_space(well)
    cnt = 10000;
    if isempty(well)
        
        well(cnt,1).Eid = [];
        well(cnt,1).Sid = [];
        well(cnt,1).p_cds = [];
        well(cnt,1).v_cds = [];
        well(cnt,1).p_lnd = [];
        well(cnt,1).v_lnd = [];
        well(cnt,1).URF = [];
    else
        Nsize = size(well,1);
        well(Nsize+cnt,1).Eid = [];
        well(Nsize+cnt,1).Sid = [];
        well(Nsize+cnt,1).p_cds = [];
        well(Nsize+cnt,1).v_cds = [];
        well(Nsize+cnt,1).p_lnd = [];
        well(Nsize+cnt,1).v_lnd = [];
        well(Nsize+cnt,1).URF = [];
    end
end

